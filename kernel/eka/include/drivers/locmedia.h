// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\locmedia.h
// 
//

#ifndef LOCMEDIA_H
#define LOCMEDIA_H
#include <plat_priv.h>
#include <d32locd.h>

#if defined(_DEBUG) && defined(__DEMAND_PAGING__)
#define __CONCURRENT_PAGING_INSTRUMENTATION__
#endif
#ifdef __DEMAND_PAGING__
#define __DEMAND_PAGING_BENCHMARKS__
#endif

#ifdef __WINS__
	#define __EMULATOR_DMA_SUMULATION__
#endif

/**
@publishedPartner
@released

A media driver priority value.

The value can be returned by a media driver's PDD factory Info() function,
and allows Symbian OS to decide the order in which media drivers are to be opened.

The value is relative to the other media driver priority values.

@see DPhysicalDevice::Info()
*/
const TInt KMediaDriverPriorityHigh=2;




/**
@publishedPartner
@released

A media driver priority value.

The value can be returned by a media driver's PDD factory Info() function,
and allows Symbian OS to decide the order in which media drivers are to be opened.

The value is relative to the other media driver priority values, and is
the one most commonly used.

@see DPhysicalDevice::Info()
*/
const TInt KMediaDriverPriorityNormal=1;




/**
@publishedPartner
@released

A media driver priority value.

The value can be returned by a media driver's PDD factory Info() function,
and allows Symbian OS to decide the order in which media drivers are to be opened.

The value is relative to the other media driver priority values.

@see DPhysicalDevice::Info()
*/
const TInt KMediaDriverPriorityLow=0;




/**
@publishedPartner
@released

Media driver interface major version number.
*/
const TInt KMediaDriverInterfaceMajorVersion=1;




/**
@publishedPartner
@released

Media driver interface minor version number.
*/
const TInt KMediaDriverInterfaceMinorVersion=0;





/**
@publishedPartner
@released

Media driver interface build number.
*/
const TInt KMediaDriverInterfaceBuildVersion=160;




/**
@publishedPartner
@released
*/
const TInt KMediaDriverDeferRequest=1;




/**
@internalTechnology
*/
#define __TRACE_TIMING(x)
//#define __TRACE_TIMING(x) *(TInt*)0x63000ff0=x

/**
@internalComponent
*/
NONSHARABLE_CLASS(DLocalDriveFactory) : public DLogicalDevice
	{
public:
	DLocalDriveFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class TLocDrv;
class DLocalDrive;

/**
@internalComponent
*/
NONSHARABLE_CLASS(TLocalDriveCleanup) : public TThreadCleanup
	{
public:
	TLocalDriveCleanup();
	virtual void Cleanup();
	inline DLocalDrive& LocalDrive();
	};



class TLocDrvRequest;
class DPrimaryMediaBase;

/* 
TCallBackLink

@internalComponent

Internal class which allows a list of callbacks to be linked together.
*/

NONSHARABLE_CLASS(TCallBackLink)
	{
public:
	enum TObjectType
		{
		EDLocalDriveObject, // object containing this TCallBackLink is a DLocalDrive
		ETLocDrvObject,		// object containing this TCallBackLink is a TLocDrv
		};

public:
	TCallBackLink();
	TCallBackLink(TInt (*aFunction)(TAny* aPtr, TInt aParam),TAny* aPtr, TObjectType aObjectType);
	TInt CallBack(TInt aParam) const;
public:
	/**
	A pointer to the callback function.
	*/
	TInt (*iFunction)(TAny* aPtr, TInt aParam);
	
	
	/**
	A pointer that is passed to the callback function when
	the function is called.
	*/
	TAny* iPtr;

	TObjectType iObjectType;

	SDblQueLink iLink;
	};


/**
@publishedPartner
@released

This class is strictly internal to Symbian; the only part of this class that
is publicly exposed to partners is the TRequestId enum.

@see DLocalDrive::TRequestId
*/
NONSHARABLE_CLASS(DLocalDrive) : public DLogicalChannelBase
	{
public:
	/**
	Identifies the specific local drive operation.
	*/
	enum TRequestId
		{
		/**
		Requests information about the size, type, and attributes of the media.
		*/
		ECaps=0,
		
		/**
		Requests an asynchronous read from the media device.
		*/
		ERead=1,
		
		/**
		Requests an asynchronous write to the media device.
		*/
		EWrite=2,
		
		/**
		Requests the formatting of a section of the media
		*/
		EFormat=3,
		
		/**
		A request to expand the total size of the media.
		*/
		EEnlarge=4,
		
		/**
		A request to reduce the total size of the media.
		*/
		EReduce=5,
		
		/**
		A request to force a remount of the media.
		*/
		EForceMediaChange=6,
		
		/**
		Requests an attempt to lock the media with a password.
		*/
		EPasswordLock=7,
		
		/**
		Requests an attempt to unlock the media.
		*/
		EPasswordUnlock=8,
		
		/**
		Requests an attempt to remove the password from the media.
		*/
		EPasswordClear=9,
		
		/**
		Requests an read of the password store.
		*/
		EReadPasswordStore=10,
		
		/**
		Requests a write of the password store.
		*/
		EWritePasswordStore=11,
		
		/** 
		A request to get the length of the password store.
		*/
		EPasswordStoreLengthInBytes=12,
		/** 
		A Control IO request
		*/
		EControlIO=13,
		/** 
		A request to force an erase of the password from the media
		*/
		EPasswordErase=14,

		/** 
		A delete notification from the file system
		*/
		EDeleteNotify=15,

		/** 
		A request for information on the last error
		*/
		EGetLastErrorInfo=16,

		EFirstReqNumberReservedForPaging=17,
		// DO NOT REUSE ANY OF THE REQUEST NUMBERS BETWEEN THIS AND THE LAST RESERVED REQ NUMBER
		// ALSO DO NOT INSERT ANY REQUEST NUMBERS BEFORE THIS, AS THIS WILL BE A COMPATIBILITY BREEAK
		ELastReqNumberReservedForPaging=31,

		/**
		Query device 
		*/
		EQueryDevice=32,
		};
public:
	DLocalDrive(); 
	~DLocalDrive();

	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer); /**< @internalComponent */
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);                      /**< @internalComponent */

	void NotifyChange();

	inline void Deque();                 /**< @internalComponent */

	static TInt MediaChangeCallback(TAny* aLocalDrive, TInt aNotifyType);	/**< @internalComponent */

	IMPORT_C static TInt Caps(TInt aDriveNumber, TDes8& aCaps);

private:
#ifdef __DEMAND_PAGING__
	TInt LockMountInfo(DPrimaryMediaBase& aPrimaryMedia, TLocDrvRequest& aReq);
	void UnlockMountInfo(DPrimaryMediaBase& aPrimaryMedia);
#endif
	TInt ReadPasswordData(TLocDrvRequest& aReq, TLocalDrivePasswordData& aPswData, TMediaPassword& aOldPasswd, TMediaPassword& aNewPasswd);

public:
	TLocDrv* iDrive;									/**< @internalComponent */
	TCallBackLink iMediaChangeObserver;					/**< @internalComponent */
	TClientDataRequest<TBool>* iNotifyChangeRequest;	/**< @internalComponent */
	TLocalDriveCleanup iCleanup;						/**< @internalComponent */
	};

/**
@internalComponent
*/
inline DLocalDrive& TLocalDriveCleanup::LocalDrive()
	{ return *_LOFF(this,DLocalDrive,iCleanup); }




/**
@publishedPartner
@released

A class that encapsulates the request information received from the client,
and gives the media driver access to the request ID and other associated
parameters, such as the request length, offset, the requesting thread,
source and destination address etc.

An object of this type is passed to DMediaDriver::Request().

@see DMediaDriver::Request()
*/
class TLocDrvRequest : public TThreadMessage
	{
public:

    /** 
	@internalComponent
    */
	enum TFlags
		{
		EWholeMedia=1,
		EAdjusted=2,
		EPhysAddr=0x04,
		EPaging=0x08,				// a paging request
		EBackgroundPaging=0x10,		// a background paging request. @see DMediaPagingDevice::Write()
		ECodePaging=0x20,			// a code paging request
		EDataPaging=0x40,			// a data paging request
		ETClientBuffer=0x80,		// RemoteDes() points to a TClientBuffer
		EKernelBuffer=0x100,		// RemoteDes() points to a kernel-side buffer : set for all paging requests and media extension requests
		EPhysAddrOnly=0x200,        // No virtual address is available. Data Paging requests Only. 
		};
public:
    
    /**
    Gets a reference to the object containing the request information.
    
    @return The request information.
    */
	inline static TLocDrvRequest& Get()
		{return (TLocDrvRequest&)Kern::Message();}
	
		
    /**
    Gets the request ID.
    
    For media drivers, this is one of the DLocalDrive::TRequestId enumerated values.
    
    @return The request ID.
    
    @see DLocalDrive::TRequestId
    */
	inline TInt& Id()
		{return *(TInt*)&iValue;}
		
		
    /**
    Gets the position on the media on which the request operates. 
    
    This applies to operations ERead, EWrite and EFormat.

    Note that the partition offset is taken into account by the underlying
    local media subsystem.
    
    @return The position on the media.
    
    @see TRequestId::ERead
    @see TRequestId::EWrite
    @see TRequestId::EFormat
    */		
	inline Int64& Pos()
		{return *(Int64*)&iArg[0];}
	
	
	/**
	Gets the length associated with the operation.
	
	This is the number of bytes associated with the media request.
	It applies to operations ERead, EWrite and EFormat.
	
	@return The length, in bytes.
	
    @see TRequestId::ERead
    @see TRequestId::EWrite
    @see TRequestId::EFormat
	*/
	inline Int64& Length()
		{return *(Int64*)&iArg[2];}
		
	
	/**
	Gets a pointer to the remote thread that requested the operation.
	
	This may be used to access the data to be read from the remote thread's process,
	or the area to which data is to be written in the remote thread's process.
	However, it is recommended that	such operations be performed
	using ReadRemote() and WriteRemote()
	
	@return A reference to a pointer to the remote thread.
	
	@see TLocDrvRequest::ReadRemote()
	@see TLocDrvRequest::WriteRemote()
	*/
	inline DThread*& RemoteThread()
		{return *(DThread**)&iArg[4];}
		
		
	/**
	Gets a pointer to the descriptor in the remote thread's process that
	contains the data to be read, or is the target for data to be written.
	
    However, it is recommended that such read or write operations be performed
    using ReadRemote() and WriteRemote().
    
    @return A reference to a pointer to the remote descriptor.

    @see TLocDrvRequest::ReadRemote()
	@see TLocDrvRequest::WriteRemote()
	*/	
	inline TAny*& RemoteDes()
		{return *(TAny**)&iArg[5];}
		
		
    /**
    Gets the offset within the descriptor in the remote thread's process.
    
    @return The offset within the descriptor.
    */
	inline TInt& RemoteDesOffset()
		{return *(TInt*)&iArg[6];}
		
		
	/**
	@internalComponent
	*/
	inline TInt& Flags()
		{return *(TInt*)&iArg[7];}
		
		
	/**
	@internalComponent
	*/
	inline TLocDrv*& Drive()
		{return *(TLocDrv**)&iArg[8];}
		
		
	/**
	@internalComponent
	*/
	inline TInt& DriverFlags()
		{return *(TInt*)&iArg[9];}


	/**
	Returns true if Physical memory addresses are available for this TLocDrvRequest.
	@return ETrue if a physical memory address is available.
	*/
	inline TBool IsPhysicalAddress()
		{return Flags() & EPhysAddr;}
public:
	TInt ProcessMessageData(TAny* args);
	void CloseRemoteThread();
	IMPORT_C TInt ReadRemote(TDes8* aDes, TInt anOffset);
	IMPORT_C TInt ReadRemote(const TAny* aSrc, TDes8* aDes);
	IMPORT_C TInt ReadRemoteRaw(TAny* aDes, TInt aSize);
	IMPORT_C TInt WriteRemote(const TDesC8* aDes, TInt anOffset);
	IMPORT_C TInt WriteRemoteRaw(const TAny* aSrc, TInt aSize);
	IMPORT_C TInt CheckAndAdjustForPartition();
#if !defined(__WINS__)
	IMPORT_C TInt WriteToPageHandler(const TAny* aSrc, TInt aSize, TInt anOffset);
	IMPORT_C TInt ReadFromPageHandler(TAny* aDst, TInt aSize, TInt anOffset);
#endif // __WINS__
	IMPORT_C TInt GetNextPhysicalAddress(TPhysAddr& aPhysAddr, TInt& aLength);
	};




/**
@internalComponent
*/
inline void DLocalDrive::Deque()
	{ iMediaChangeObserver.iLink.Deque(); }




/**
@publishedPartner
@released

Defines a structure used to contain information that describes an individual
partition. 

There is one of these for each partition that exists on a media device.

@see TPartitionInfo
*/
class TPartitionEntry
	{
public:
     
    /**
    The start address of the partition, described as a relative offset,
    in bytes, from the start of the media.
    
    This value is used by the local media subsystem to calculate
    the absolute address on the media whenever an access such as a Read,
    Write or Format request is made.
    */
	Int64 iPartitionBaseAddr;
 
 	
	/**
	The length of the partition, in bytes.
	*/
	Int64 iPartitionLen;

	
	/**
	The Boot Indicator record, as described in the Master Boot Record on
	FAT Partitioned devices.

	This is currently unused by the local media subsystem.
	*/
	TUint16 iBootIndicator;
	
	
	/**
	Describes the type of partition.
	
	The File Server uses this to decide the type of filesystem to be mounted
	on the partition. 
	
	Symbian OS supports many partition types, as defined in partitions.h.
	You are, however, free to invent your own partition type, on which
	you could, for example, mount your own filesystem. However, make sure 
	that your partition type does not clash with an existing partition type.
	
	Note that a media driver does not does not have to verify that
	the partition actually contains a file system of this type; it just sets
	this value to indicate the intended use for this partition.
	*/
	TUint16 iPartitionType;
	};




/**
@publishedPartner
@released

A constant that defines the maximum number of partitions that can exist on
a media device.

@see TPartitionInfo::iPartitionCount
*/
const TInt KMaxPartitionEntries=0x10;




/**
@publishedPartner
@released

Contains partition information for a media device.

An object of this type is passed to the media driver's implementation of
DMediaDriver::PartitionInfo() to be filled in.

@see DMediaDriver::PartitionInfo()
*/
class TPartitionInfo
	{
public:

    /**
    Default constructor that clears this object's memory to binary zeroes.
    */
	TPartitionInfo();
public:

    /**
    The total size of the media, in bytes.
    */
	Int64 iMediaSizeInBytes;
	
	
	/**
	The total number of partitions that exist on the media.
	
	This is always less than or equal to KMaxPartitionEntries.
	
    @see KMaxPartitionEntries
	*/
	TInt iPartitionCount;
	
	
	/**
	Information that describes each individual partition on the device.
	
	Each partition is represented by an array of TPartitionEntry objects. 
	Each entry must be created in the order of the start offset, so that 
	iEntry[0] specifies the partition with
	the smallest iPartitionBaseAddr value.
	
	@see TPartitionEntry::iPartitionBaseAddr
	@see TPartitionEntry
	*/
	TPartitionEntry iEntry[KMaxPartitionEntries];
	};




class DMedia;
class DPrimaryMediaBase;
class DMediaDriver;
#ifdef __DEMAND_PAGING__
class DFragmentationPagingLock;
#endif
class DDmaHelper;

/**
@internalComponent
*/
class TLocDrv : public TPartitionEntry
	{
public:
	TLocDrv(TInt aDriveNumber);
public:
	inline TInt Connect(DLocalDrive* aLocalDrive);
	inline void Disconnect(DLocalDrive* aLocalDrive);
	inline TInt Request(TLocDrvRequest& aRequest);
	static TInt MediaChangeCallback(TAny* aLocDrv, TInt aNotifyType);
public:
	TInt iDriveNumber;
	DMedia* iMedia;
	DPrimaryMediaBase* iPrimaryMedia;
	TInt iPartitionNumber;
	TErrorInfo iLastErrorInfo;
#ifdef __DEMAND_PAGING__
	TInt iSpare0;
	TUint8 iPagingDrv;
	TUint8 iSpare1;
	TUint8 iSpare2;
	TUint8 iSpare3;
#endif
	DDmaHelper* iDmaHelper;

	// Media extension stuff:

	/** ptr to the next TLocDrv object in the chain. Null if not a media extension */
	TLocDrv* iNextDrive;

	/** media change callback - called when the next media in the chain has a media change */
	TCallBackLink iMediaChangeObserver;
	};

/**
@publishedPartner
@released

Kernel-side representation of a media entity. A DMedia object instantiates a 
media driver to provide access to the physical media.

Multiple DMedia objects may be required by some devices, e.g. multi-media cards
with combined SRAM and Flash or cards containing user and protected areas.
*/
class DMedia : public DBase
	{
public:
	/**
	Declaration of all legal states for the media driver.
	*/
	enum TMediaState
		{
		/** 
		Media is powered down and the media drivers are closed. This is the 
		initial state when a media driver is created and the final state when
		the driver is closed.
		*/
		EClosed=0,

		/** Media driver has started powerup sequence. */
		EPoweringUp1=1,

		/** Media is being opened. */
		EOpening=2,

		/** Media is open and the partition information is being read. */
		EReadPartitionInfo=3,
		
		/** The media driver is open and ready to accept commands. */
		EReady=4,

		/** Not used. */
		EAborted=5,

		/** Powering up with media drivers open. */
		EPoweringUp2=6,

		/** Not used. */
		ERecovering=7,

		/** The media is powered down, but the media driver still exists. */
		EPoweredDown=8 // powered down with media drivers open
		};

public:
	IMPORT_C virtual TInt Create(TMediaDevice aDevice, TInt aMediaId, TInt);
	void Close();
public:
	inline Int64 MediaLenInBytes();
	inline TMediaDevice DeviceType();
	inline TInt PartitionCount();
	inline Int64 PartitionBaseAddr(TInt aPartition);
	inline Int64 PartitionLen(TInt aPartition);
public:
    /** Not used. */
	static TInt MediaCallBack(TAny *aPtr);
public:
	/**
	The unique ID associated with this media entity. 
	ID allocated when the media is first created.
	
	@see LocDrv::RegisterMediaDevice
	*/
	TInt iMediaId;

	/**
	The unique ID for the device.

	@see TMediaDevice
	*/
	TMediaDevice iDevice;

	/**
	Partition information for the media device.

	@see TPartitionInfo
	*/
	TPartitionInfo iPartitionInfo;

	/**
	The media's physical device driver.

	@see DMediaDriver
	*/
	DMediaDriver *iDriver;

	/**
	Mount information for the media device.
	
	@see TMountInfoData
	*/
	TMountInfoData iMountInfo;
	};

#ifdef __DEMAND_PAGING__
class DFragmentationPagingLock;
class DMediaPagingDevice;
#endif


/**
@publishedPartner
@released
The DPrimaryMedia base class which is derived from DMedia class is responsible for controlling the overall state of the media 
(for example whether the power is applied or the partition information has been determined and so on). 
Each media driver or extension that registers for a set of local drives also has to register for a set of DMedia objects at the same time.
This media set must contain just one primary media object. 
The driver that performs drive registration is responsible for creating the primary media object itself, 
which it then passes over to the local media sub-system for ownership. 
If further media objects are specified in the set, then the local media sub-system itself creates DMedia instances for these on behalf of the driver.
*/
class DPrimaryMediaBase : public DMedia
	{
public:
	enum TMsgId
		{
		EConnect=-1,
		EDisconnect=-2,
		};

	enum TForceMediaChangeFlags
		{
		/**
		Specifying zero as the flag for DPrimaryMediaBase::ForceMediaChange(), 
		results in all media drivers associated with the primary media being 
		closed and reopened.
		All pending requests on all logical drives associated with the primary 
		media will be cancelled.

		@see DPrimaryMediaBase::ForceMediaChange()
		@see RLocalDrive::ForceMediaChange()
		*/
		KForceMediaChangeReOpenAllMediaDrivers = 0,
		/**
		This flag is used to simulate ejecting and re-inserting the media.
		All pending requests on all logical drives associated with the primary 
		media will be cancelled.
		N.B. This is asynchronous in behaviour i.e. the caller will need to wait 
		for (two) media change notifications before the drive is ready for use

		@see DPBusPrimaryMedia::ForceMediaChange()
		@see RLocalDrive::ForceMediaChange()
		*/
		KMediaRemountForceMediaChange = 0x00000001,
		/**
		This flag is used to force the media driver for the specified logical 
		drive to be closed and reopened.
		It should not affect any pending requests on other logical drives 
		associated with the primary media.

		@see DPrimaryMediaBase::ForceMediaChange()
		@see RLocalDrive::ForceMediaChange()
		*/
		KForceMediaChangeReOpenMediaDriver = 0x80000000
		};

public:
	IMPORT_C DPrimaryMediaBase();

	// provided by implementation
	IMPORT_C virtual TInt Create(TMediaDevice aDevice, TInt aMediaId, TInt aLastMediaId);
	IMPORT_C virtual TInt Connect(DLocalDrive* aLocalDrive);
	IMPORT_C virtual void Disconnect(DLocalDrive* aLocalDrive);
	IMPORT_C virtual TInt Request(TLocDrvRequest& aRequest);
	IMPORT_C virtual TInt QuickCheckStatus();
	IMPORT_C virtual TInt ForceMediaChange(TInt aMode);
	IMPORT_C virtual TInt InitiatePowerUp();
	IMPORT_C virtual TInt DoInCritical();
	IMPORT_C virtual void DoEndInCritical();
	IMPORT_C virtual void DeltaCurrentConsumption(TInt aCurrent);
	IMPORT_C virtual void DefaultDriveCaps(TLocalDriveCapsV2& aCaps);
	IMPORT_C virtual TBool IsRemovableDevice(TInt& aSocketNum);

	// used by implementation
	IMPORT_C void NotifyMediaChange();
	IMPORT_C void NotifyPowerDown();
	IMPORT_C void NotifyEmergencyPowerDown();
	IMPORT_C void NotifyPsuFault(TInt anError);
	IMPORT_C void NotifyMediaPresent();
	IMPORT_C void PowerUpComplete(TInt anError);

	IMPORT_C virtual void HandleMsg(TLocDrvRequest& aRequest);
	IMPORT_C virtual TInt DoRequest(TLocDrvRequest& aRequest);
	TInt OpenMediaDriver();
	void CloseMediaDrivers(DMedia* aMedia = NULL);
	void StartOpenMediaDrivers();
	void OpenNextMediaDriver();
	void DoOpenMediaDriverComplete(TInt anError);
	void DoPartitionInfoComplete(TInt anError);
	void CompleteCurrent(TInt anError);
	void CompleteRequest(TLocDrvRequest& aMsg, TInt aResult);
	IMPORT_C void RunDeferred();
	void SetClosed(TInt anError);
	
	enum TNotifyType {EMediaChange, EMediaPresent};
	void NotifyClients(TNotifyType aNotifyType, TLocDrv* aLocDrv=NULL);

	TInt InCritical();
	void EndInCritical();
	void UpdatePartitionInfo();
	void MediaReadyHandleRequest();
	TInt SendReceive(TLocDrvRequest& aReq, TLinAddr aLinAddress = NULL);

#ifdef __DEMAND_PAGING__
	TInt PinSendReceive(TLocDrvRequest& aReq, TLinAddr aStart = NULL);
	TInt PinFragmentSendReceive(TLocDrvRequest& aReq, TLinAddr aLinAddress, TInt aLength);

	TBool PagingMediaPinAddress(TLinAddr aLinAddress, TInt aSize);
	void PagingMediaUnpinAddress();
#endif

#ifdef __DEMAND_PAGING__
	void RequestCountInc();
	void RequestCountDec();
#endif

	// called by LocDrv::RegisterMediaDevice() for media extensions
	TInt Connect(TLocDrv* aLocDrv);

	void MediaChange();
	TInt HandleMediaNotPresent(TLocDrvRequest& aReq);


public:
	TInt iLastMediaId;					/**< @internalComponent */
	TMessageQue iMsgQ;
	TDfcQue* iDfcQ;
	SDblQue iConnectionQ;				/**< @internalComponent */
	TMessageQue iDeferred;				/**< @internalComponent */
	TMessageQue iWaitMedChg;			/**< @internalComponent */
	TInt iState;						/**< @internalComponent */
	TInt iCritical;						/**< @internalComponent */
	TLocDrvRequest* iCurrentReq;
	TDfc iAsyncDfc;						/**< @internalComponent */
	TInt iAsyncErrorCode;				/**< @internalComponent */
	RPhysicalDeviceArray iPhysDevArray;	/**< @internalComponent */

	class DBody;
	DBody* iBody;						/**< @internalComponent */

	TInt iNextMediaId;					/**< @internalComponent */
	TInt iTotalPartitionsOpened;		/**< @internalComponent */
	TInt iMediaDriversOpened;			/**< @internalComponent */
	DMediaDriver* iNextMediaDriver;		/**< @internalComponent */


#ifdef __DEMAND_PAGING__
	// keep the size of class as it is used as base for PBus and may not want to bother building DP specific version.
	TUint8 iPagingMedia;				/**< @internalComponent */
	TUint8 iDataPagingMedia;			/**< @internalComponent */
	TUint8 iRomPagingMedia;				/**< @internalComponent */
	TUint8 iRunningDeferred;			/**< @internalComponent */
#else
	TInt iRunningDeferred;				/**< @internalComponent */
#endif
	};

#ifdef __DEMAND_PAGING__

/**
@internalComponent
@prototype
*/
NONSHARABLE_CLASS(DMediaPagingDevice) : public DPagingDevice
	{
public:
	enum TPagingRequestId
		{
		/** 
		Identifies any middle fragment of a Write request on a partition of a media that supports paging.
		@deprecated
		*/
		EWriteRequestFragment		=	DLocalDrive::EFirstReqNumberReservedForPaging,

		/** 
		Identifies the last fragment of a Write request on a partition of a media that supports paging.
		@deprecated
		*/
		EWriteRequestFragmentLast	=	DLocalDrive::EFirstReqNumberReservedForPaging+1,

		/** 
		Request for paging in (read) data from the ROM store area.
		*/
		ERomPageInRequest			=	DLocalDrive::EFirstReqNumberReservedForPaging+2,

		/** 
		Request for paging in (read) data from the code store area.
		*/
		ECodePageInRequest			=	DLocalDrive::EFirstReqNumberReservedForPaging+3,

        /**
        Provided to allow the following compile time assert.
        */
		EPagingRequestHighWaterMark
		};
    __ASSERT_COMPILE(EPagingRequestHighWaterMark <= DLocalDrive::ELastReqNumberReservedForPaging + 1);

	enum TQueue
		{
		EMainQ = 0x01,
		EDeferredQ=0x02
		};

public:
	DMediaPagingDevice(DPrimaryMediaBase* aPtr);
	virtual ~DMediaPagingDevice();
	
	// from DPagingDevice
	virtual TInt Read(TThreadMessage* aReq,TLinAddr aBuffer,TUint aOffset,TUint aSize,TInt aDrvNumber);
	virtual TInt Write(TThreadMessage* aReq,TLinAddr aBuffer,TUint aOffset,TUint aSize, TBool aBackground);
	virtual TInt DeleteNotify(TThreadMessage* aReq,TUint aOffset,TUint aSize);

	void CompleteRequest(TThreadMessage* aMsg, TInt aResult);
	void SendToMainQueueDfcAndBlock(TThreadMessage* aMsg);
	void SendToDeferredQ(TThreadMessage* aMsg);
	inline static TBool PageInRequest(TLocDrvRequest& aReq);
	inline static TBool PageOutRequest(TLocDrvRequest& aReq);
	inline static TBool PagingRequest(TLocDrvRequest& aReq);
		
	virtual TInt WritePhysical(TThreadMessage* aReq, TPhysAddr* aPageArray, TUint aPageCount, TUint aOffset, TBool aBackground);
	virtual TInt ReadPhysical(TThreadMessage* aReq, TPhysAddr* aPageArray, TUint aPageCount, TUint aOffset, TInt aDrvNumber);
private:    
    virtual TInt BaseRead(TThreadMessage* aReq,TUint32 aBuffer,TUint aOffset,TUint aSize,TInt aDrvNumber,TBool aPhysAddr);
    virtual TInt BaseWrite(TThreadMessage* aReq,TUint32 aBuffer,TUint aOffset,TUint aSize, TBool aBackground,TBool aPhysAddr); 
public:
	TMessageQue iMainQ;
	TMessageQue iDeferredQ;
	DPrimaryMediaBase* iPrimaryMedia;

	TUint8 iEmptyingQ;
	TUint8 iDeleteNotifyNotSupported;
	TUint8 iSpare1;
	TUint8 iSpare2;

	TAny* iMountInfoDataLock;
	TAny* iMountInfoDescHdrLock;
	TAny* iMountInfoDescLenLock;

	TInt iFirstLocalDriveNumber;
	TInt iRomPagingDriveNumber;
	TInt iDataPagingDriveNumber;

	NFastMutex iInstrumentationLock;			// To protect instrumentation data

#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
	TUint8 iServicingROM;
	TUint8 iServicingCode;
	TUint8 iServicingDataIn;
	TUint8 iServicingDataOut;

	SMediaROMPagingConcurrencyInfo iROMStats;
	SMediaCodePagingConcurrencyInfo iCodeStats;
	SMediaDataPagingConcurrencyInfo iDataStats;
#endif

#ifdef __DEMAND_PAGING_BENCHMARKS__
	SPagingBenchmarkInfo iROMBenchmarkData;
	SPagingBenchmarkInfo iCodeBenchmarkData;
	SPagingBenchmarkInfo iDataInBenchmarkData;
	SPagingBenchmarkInfo iDataOutBenchmarkData;
	SMediaPagingInfo iMediaPagingInfo;
#endif
	};

inline TBool DMediaPagingDevice::PageInRequest(TLocDrvRequest& aReq)
	{
	return 
		(aReq.Flags() & TLocDrvRequest::EPaging) && 
		(aReq.Id() == ERomPageInRequest || 
		 aReq.Id() == ECodePageInRequest || 
		 aReq.Id() == DLocalDrive::ERead);}

inline TBool DMediaPagingDevice::PageOutRequest(TLocDrvRequest& aReq)
	{
	return 
		(aReq.Flags() & TLocDrvRequest::EPaging) && 
		(aReq.Id() == DLocalDrive::EWrite);}

inline TBool DMediaPagingDevice::PagingRequest(TLocDrvRequest& aReq)
	{
	return (aReq.Flags() & TLocDrvRequest::EPaging);
	}


/**
@internalComponent
@prototype
*/
class DFragmentationPagingLock: public DDemandPagingLock
	{
public:
	TInt Construct(TUint aNumPages);
	void Cleanup();
	void LockFragmentation()
		{
		__ASSERT_CRITICAL;
		__ASSERT_DEBUG(iFragmentationMutex, Kern::Fault("LOCMEDIA_H",__LINE__));
		// called in CS
		Kern::MutexWait(*iFragmentationMutex);
		}
	void UnlockFragmentation()
		{
		__ASSERT_CRITICAL;
		__ASSERT_DEBUG(iFragmentationMutex, Kern::Fault("LOCMEDIA_H",__LINE__));
		// called in CS
		Kern::MutexSignal(*iFragmentationMutex);
		}


public:
	TUint iFragmentGranularity;
private:
	DMutex* iFragmentationMutex; // to protect Kernel memory locking
	};
#endif //__DEMAND_PAGING__

/**
@publishedPartner
@released

An abstract base class for all media drivers in the local drive system.

All media drivers, whether associated with fixed media, such as the internal
drive, or removable media, such as a PC Card or MultiMediaCard, must define
and implement a class derived from this one.

An instance of this class is created by the media driver's PDD factory,
an instance of a class derived from DPhysicalDevice. 

@see DPhysicalDevice::Create()
@see DPhysicalDevice
*/
class DMediaDriver : public DBase
	{
public:
	IMPORT_C DMediaDriver(TInt aMediaId);
	IMPORT_C virtual ~DMediaDriver();
	IMPORT_C virtual void Close();
// Pure virtual
	IMPORT_C virtual void Disconnect(DLocalDrive* aLocalDrive, TThreadMessage* aMsg);
	
	/**
	A function called by the local media subsystem to deal with a request, 
	and which must be implemented by the media driver.
	
	@param aRequest An object that encapsulates information about the request.
	
	@return A value indicating the result:
	        KErrNone, if the request has been sucessfully initiated;
	        KErrNotSupported, if the request cannot be handled by the device;
	        KMediaDriverDeferRequest, if the request cannot be handled
	        immediately because of an outstanding request (this request will be
	        deferred until the outstanding request has completed);
	        otherwise one of the other system-wide error codes.
	*/
	virtual TInt Request(TLocDrvRequest& aRequest)=0;

	/**
	A function called by the local media subsystem to get partition information
	for the media device.
	
	It is called once the subsystem has been notified that the media driver
	is open and has been succesfully initialised.
	
	This function must be implemented by the media driver.
	
	@param anInfo An object that, on successful return, contains
	              the partition information.
	
	@return KErrNone, if retrieval of partition information is to be
	        done asynchronously;
	        KErrCompletion, if retrieval of partition information has been
	        done synchronously, and successfully;
	        one of the other system-wide error codes, if retrieval of partition
	        information has been done synchronously, but unsuccessfully.
	*/
	virtual TInt PartitionInfo(TPartitionInfo &anInfo)=0;

	/**
	A function called by the local media subsystem to inform the media driver
	that the device should power down.
	
	This function must be implemented by the media driver.
	*/
	virtual void NotifyPowerDown()=0;

	/**
	A function called by the local media subsystem to inform the media driver
	that the device is to be immediately powered down.
	
    This function must be implemented by the media driver.
	*/
	virtual void NotifyEmergencyPowerDown()=0;
public:
	IMPORT_C void SetTotalSizeInBytes(Int64 aTotalSizeInBytes, TLocDrv* aLocDrv=NULL);
	IMPORT_C void SetTotalSizeInBytes(TLocalDriveCapsV4& aCaps);
	IMPORT_C Int64 TotalSizeInBytes();
	IMPORT_C void SetCurrentConsumption(TInt aValue);
	IMPORT_C TInt InCritical();
	IMPORT_C void EndInCritical();
	IMPORT_C void Complete(TLocDrvRequest& aRequest, TInt aResult);
	IMPORT_C void OpenMediaDriverComplete(TInt anError);
	IMPORT_C void PartitionInfoComplete(TInt anError);
public:
	DPhysicalDevice* iPhysicalDevice;/**< @internalComponent */
	Int64 iTotalSizeInBytes;         /**< @internalComponent */
    TInt iCurrentConsumption;        /**< @internalComponent */
	DPrimaryMediaBase* iPrimaryMedia;/**< @internalComponent */
	TBool iCritical;                 /**< @internalComponent */
	TMountInfoData* iMountInfo;      /**< @internalComponent */
	};


/**
@internalTechnology
@prototype

An abstract base class for media driver 'extensions' within the local media subsystem
*/
class DMediaDriverExtension : public DMediaDriver
	{
public:
	IMPORT_C DMediaDriverExtension(TInt aMediaId);
	IMPORT_C virtual ~DMediaDriverExtension();
	IMPORT_C virtual void Close();

	virtual TInt Request(TLocDrvRequest& aRequest) = 0;
	
	virtual TInt PartitionInfo(TPartitionInfo &anInfo) = 0;

	IMPORT_C virtual void NotifyPowerDown();

	IMPORT_C virtual void NotifyEmergencyPowerDown();

	/**
	Retrieve partition info from all the attached drives
	*/
	IMPORT_C TInt DoDrivePartitionInfo(TPartitionInfo &anInfo);
	/**
	Forward a request to the next attached drive
	*/
	IMPORT_C TInt ForwardRequest(TLocDrvRequest& aRequest);

	/**
	Read from the specified attached drive
	*/
	IMPORT_C TInt Read(TInt aDriveNumber, TInt64 aPos, TLinAddr aData, TUint aLen);

	/**
	Write to the specified attached drive
	*/
	IMPORT_C TInt Write(TInt aDriveNumber, TInt64 aPos, TLinAddr aData, TUint aLen);

	/**
	Get the Caps from the specified attached drive
	*/
	IMPORT_C TInt Caps(TInt aDriveNumber, TDes8& aCaps);

	/**
	Return whether the media is busy i.e. if it has any pending requests or DFCs
	*/
	IMPORT_C TBool MediaBusy(TInt aDriveNumber);

#ifdef __DEMAND_PAGING__
	/**
	Send a paging read request to the specified attached drive
	*/
	IMPORT_C TInt ReadPaged(TInt aDriveNumber, TInt64 aPos, TLinAddr aData, TUint aLen);

	/**
	Send a paging write request to the specified attached drive
	*/
	IMPORT_C TInt WritePaged(TInt aDriveNumber, TInt64 aPos, TLinAddr aData, TUint aLen);
#endif

private:
	TInt SendRequest(TInt aReqId, TBool aPagingRequest, TInt aDriveNumber, TInt64 aPos, TLinAddr aData, TUint aLen);

	};




/**
@publishedPartner
@released

A structure that a media driver may find useful in its implementation,
and is used to contain the information required when registering
the media driver with the Local Media Subsystem.

@see LocDrv::RegisterMediaDevice()
*/
struct SMediaDeviceInfo
	{
	
	/**
	The unique Media ID for a device.
	
    This can take one of the enumerated values defined
    by the TMediaDevice enum.
	*/
	TMediaDevice iDevice;
	
	
	/**
	Specifies the number of local drive objects to be assigned to the media driver.
    
    Drives that support more than one partition must specify a number greater than 1.
	*/
	TInt iDriveCount;
	
	
	/**
    A pointer to an array of TInt values, which define the drive numbers that
    are to be allocated to each partition.
    
    0 signifies Drive C, 1 signifies drive D, etc. For example, to allocate
    drive letters J and K, specify an array containing the values [7,8].
    Note that the size of this array must be the same as the value specified
    by iDriveCount.
	*/
	const TInt* iDriveList;
	
	
	/**
	Specifies the total number of DMedia objects to be associated with
	the media driver.
	
	This number includes the primary DPrimaryMedia object, plus all of
	the DMedia objects that are created for each additional drive, and
	which hold basic information about partitions.
	*/
	TInt iNumMedia; 
	
	
	/**
	A pointer to a descriptor containing the name of the media driver,
	for example: PCCard
	*/
	const TDesC* iDeviceName;
	};




/**
@publishedPartner
@released

A set of utility functions used in the management of local media drivers.
*/
class LocDrv
	{
public:
	IMPORT_C static TInt RegisterMediaDevice(TMediaDevice aDevice, TInt aDriveCount, const TInt* aDriveList, DPrimaryMediaBase* aPrimaryMedia, TInt aNumMedia, const TDesC& aName);
	IMPORT_C static TInt RegisterPasswordStore(TPasswordStore* aStore);
	IMPORT_C static TPasswordStore* PasswordStore();
#if !defined(__WINS__)
	IMPORT_C static TInt RegisterPagingDevice(DPrimaryMediaBase* aPrimaryMedia, const TInt* aPagingDriveList, TInt aDriveCount, TUint aPagingType, TInt aReadShift, TUint aNumPages);
#endif // __WINS__
	IMPORT_C static TInt RegisterDmaDevice(DPrimaryMediaBase* aPrimaryMedia,
										   TInt aMediaBlockSize, 
										   TInt aDmaMaxAddressable,
										   TInt aDmaAlignment);
	};

/**
@internalComponent
*/
inline TInt TLocDrv::Connect(DLocalDrive* aLocalDrive)
	{ return iPrimaryMedia->Connect(aLocalDrive); }

/**
@internalComponent
*/
inline void TLocDrv::Disconnect(DLocalDrive* aLocalDrive)
	{ iPrimaryMedia->Disconnect(aLocalDrive); }

/**
@internalComponent
*/
inline TInt TLocDrv::Request(TLocDrvRequest& aRequest)
	{ return iPrimaryMedia->Request(aRequest); }

/**
Returns the length of the media, in bytes, according to the partition information.

@return Total length of the media, in bytes.

@see	TPartitionInfo
*/
inline Int64 DMedia::MediaLenInBytes()
	{return(iPartitionInfo.iMediaSizeInBytes);}

/**
Returns the unique media ID for this device.

@return	The device ID that was set in the call to DMedia::Create().
		The return value will be one of the enumerators declared in TMediaDevice

@see	TMediaDevice
*/
inline TMediaDevice DMedia::DeviceType()
	{return(iDevice);}

/**
Returns the total number of partitions that exist on the media according to the
partition information.

This will always be less than or equal to KMaxPartitionEntries.

@return Number of partitions that exist on the media.

@see	KMaxPartitionEntries
@see	TPartitionInfo
*/
inline TInt DMedia::PartitionCount()
	{return(iPartitionInfo.iPartitionCount);}

/**
The start address of the partition, described as a relative offset, in bytes,
from the start of the media.

This value is used by the local media subsystem to calculate the absolute 
address on the media whenever an access such as a Read, Write or Format request
is made.

@param	aPartition	The partition whose start address is to be returned.

@return	The start address of the partition.

@see	TPartitionEntry
@see	TPartitionInfo
*/
inline Int64 DMedia::PartitionBaseAddr(TInt aPartition)
	{return(iPartitionInfo.iEntry[aPartition].iPartitionBaseAddr);}

/**
Returns the length of the partition, in bytes.

@param	aPartition	The partition whose length is to be returned.

@return	The length of the partition.

@see	TPartitionEntry
@see	TPartitionInfo
*/
inline Int64 DMedia::PartitionLen(TInt aPartition)
	{return(iPartitionInfo.iEntry[aPartition].iPartitionLen);}


/**
@internalTechnology

A utility class for scanning MBR/EBR partition tables.
*/
class TPartitionTableScanner
	{
public:
	enum {ESectorShift=9, ESectorSize=512};
	enum {EMaxNest=4};
	struct SPart
		{
		SPart(const TUint8* a);
		TUint8 iBootInd;
		TUint8 iType;
		TUint32 iRSS;
		TUint32 iSectors;
		};
	struct SEBR
		{
		TInt64 iRSS;
		TInt64 iSectors;
		};
public:
	IMPORT_C void Set(TUint8* aSectorBuffer, TPartitionEntry* aEntry, TInt aMaxPartitions, TInt64 aMediaSize);
	IMPORT_C TInt64 NextLBA();
	IMPORT_C TInt NumberOfPartitionsFound() const;
	TInt MakeEntry(const SPart& aP);
public:
	TInt64 iMediaSize;				// Total media size in sectors
	TInt64 iLBA;					// LBA currently in sector buffer
	TInt64 iFirstEBR;				// LBA of first EBR if any
	SEBR iStack[EMaxNest];
	TInt iStackPointer;
	TUint8* iSectorBuffer;			// Pointer to 512 byte area where sector data is stored
	TPartitionEntry* iFirstEntry;	// Where first scanned partition is stored
	TPartitionEntry* iNextEntry;	// Where next scanned partition will be stored
	TPartitionEntry* iLimit;		// iFirstEntry + max partitions
	};



#endif
