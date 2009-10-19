// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This is the implementation of LDD. The DExDriverLogicalChannel and
// DExDriverLogicalDevice will be implemented in this file
// 
//

// include files
//
#include "exsync.h"
#include "d_exsync_ldd.h"

_LIT(KExMutexName,"ConfigDMutex");

/**
 LDD entry point
 This function is called when the LDD is loaded. This creates a factory
 object for LDD. DECLARE_STANDARD_LDD macro defines the first export to
 represent the DLL factory function, that will be called first by loader
 after loading the LDD.
 */
DECLARE_STANDARD_LDD()
	{
	// Create a LDD factory object, i.e instance of DLogicalDevice derived
	// class. This is the first step that is done after loading the driver.
	// Logical device constructor inturn creates a Logical Channel.
	//
	return new DExDriverLogicalDevice;
	}

/*
 DExDriverlLogicalChannel class implementation
 */

/**
 Logical Channel constructor
 This contructs a logical channel for the driver. This should open
 the channel between the user and the driver. A channel here, means a
 single connection between user and the driver. It should match with
 a corresponding destructor.
 */
DExDriverLogicalChannel::DExDriverLogicalChannel()
	{
	
	}

/**
 Logical Channel - DoCreate(), called by framework after creating logical
 channel object as a part of the driver loading process. Driver should do
 the required validation like capabilities check, version check etc. It
 can do any required driver level initializations here.

 @param	aUnit
 		device unit number
 @param anInfo
 		device related information
 @param aVer
 		version number
 @return	KErrNone or standard error code
 */
TInt DExDriverLogicalChannel::DoCreate(TInt aUnit,
										const TDesC8* /*anInfo*/,
										const TVersion& aVer)
	{
	// Not using anInfo, so anInfo is commented in function arguments.
	// It can also be made void it to avoid compilation warnings by doing
	// (void)anInfo;

	// As a part of platform security guidelines, every driver needs to
	// enusure capability check. This check is to ensure that the client
	// accessing the driver has the required level of trust to perform any
	// operations on the device. Any client that does not have required
	// capability as expected by the driver is returned with no permission
	// error.Kernel API Kern::CurrentThreadHasCapability(),checks if the
	// current thread has the required capability (arg1) and displays the
	// diagnostic string in case of failure.
	//
	// Check if the client has level of trust to do serial comm related
	// operations.
	if (!Kern::CurrentThreadHasCapability(ECapabilityCommDD,
				__PLATSEC_DIAGNOSTIC_STRING("Checked by Tutorial Driver")))
		return KErrPermissionDenied;

	// Check if driver version matches with the version client is trying
	// to use. Kernel API, Kern::QueryVersionSupported() verifies if the
	// versions match, returns true if test version is less than current
	// version (arg2<arg1).
	//
	if (!Kern::QueryVersionSupported(iDevice->iVersion,aVer))
		return KErrNotSupported;

	// Store the LDD object pointer in PDD. This will enable PDD to access
	// and call the LDD member functions easily. In case of callbacks to
	// LDD, PDD can use this pointer.
	//
	Pdd()->iLdd=this;

	TInt r = Kern::MutexCreate(iSyncConfigDMutex, KExMutexName, KMutexOrdGeneral0);
 	if(r!= KErrNone)
 		return r;

	// Allocate buffers for Tx and Rx. Kern::Alloc allocates a block of
	// specified size and zero fills the memory block. On success, this returns
	// pointer to the allocated buffer, else returns NULL pointer
	//
	iTxBuffer = (TUint8*) Kern::Alloc(KTxBufferSize);
	if (!iTxBuffer)
		return KErrNoMemory;
	
	iRxBuffer = (TUint8*) Kern::Alloc(KRxBufferSize);
	if (!iRxBuffer)
		return KErrNoMemory;

	// return no error
    return KErrNone;
	}

/**
 Destructor for the logical channel
 */
DExDriverLogicalChannel::~DExDriverLogicalChannel()
	{
	// If the allocated buffers for Tx and Rx are still existing delete them.
	//
	if (iTxBuffer)
		delete iTxBuffer;
	
	if (iRxBuffer)
		delete iRxBuffer;
	
	// Close the mutex handle.
 	if (iSyncConfigDMutex)
 		iSyncConfigDMutex->Close(NULL);

	
	}

/**
 Notifies an object that a handle to it has been requested by user thread.
 It allows the driver to control access to itself from user threads. It
 can decide here, if it would like to allow more than one thread/process
 to get a handle of the channel.

 @param 	aThread
 			thread object
 @param 	aType
 			owner, can be thread or process
 @return	KErrNone or standard error code
 */
TInt DExDriverLogicalChannel::RequestUserHandle(DThread* aThread,
												TOwnerType aType)
    {
    // Here, we are allowing any thread to make a request to the driver
    // There is no restriction on the ownership or the no. of users
     (void)aThread;
     (void)aType; 
        
    return KErrNone;
    }



/**
Called by the Device driver framework upon user request. 

@param aFunction	A number identifying the  message type
@param a1			A 32-bit Value passed by the user
@param a2			A 32-bit Value passed by the user

@return	KErrNone	If successful, otherwise one of the system wide error code
 */
TInt DExDriverLogicalChannel::Request(TInt aReqNo, TAny *a1, TAny *a2)
	{
	TInt r = KErrNone;
	
	KEXDEBUG(Kern::Printf("++DExDriverLogicalChannel::Request"));
	
	DThread* client;
	// Handle the synchronous requests. This is implementation specific,
	// however as a general practice, they are handled in DoControl().
	// Retrieve the current thread information using the kernel API. This
	// API gets the current Symbian OS thread, i.e reference to DThread
	// object.This thread information is required later in driver to
	// perform operations like data copy etc. across different threads.
	//
	client=&Kern::CurrentThread();


	// Check the capabilities of the thread before openning the handle.
	// As the handle sharing is allowed the client capabilities shall be checked
	// at the request level.
	// 
	if(!((client)->HasCapability(ECapabilityCommDD,
					__PLATSEC_DIAGNOSTIC_STRING("Checked by Tutorial Driver"))))
		{
		// If the required capability is not present for the user
        // thread to process this request, then deny the request.
        //
        return KErrPermissionDenied;
        }
        
	// Open kernel side refernce-counted object by calling DObject::Open()
	// DThread is derived from DObject. Opening this object increments the
	// reference count by one atomically.
	//
	((DObject*)client)->Open();
	
	r = DoControl(aReqNo, a1, a2);
	
	// Close the reference on client thread using the kernel API
	// Kern::SafeClose(). It swaps the kernel side reference counted object
	// to NULL value atomically and closes the object. This operation is
	// done in response to DObject::Close() in Logical Channel's destructor.
	//
	NKern::ThreadEnterCS();
	Kern::SafeClose((DObject*&)client,NULL);
	NKern::ThreadLeaveCS();
	
	// return result of above operations	
	return r;			
	}
	

/** 
 Handle Synchronous Requests. This is called from Request() to process
 synchronous requests, that perform the action quickly and returns.
 Handling these requests is implementation specific. However, if it needs
 any hardware operation, then PDD function can be invoked from here.
 
 @param 	aFunction
 		  	Request number
 @param 	a1
 			parameter1 passed by user for the request
 @param 	a2
 			parameter2 passed by user for the request
 			
 @return	KErrNone or standard error code
 */	
TInt DExDriverLogicalChannel::DoControl(TInt aFunction, TAny* a1,
										TAny* /*a2*/) // a2 is not being used
	{
	KEXDEBUG(Kern::Printf("++DExDriverLogicalChannel::DoControl"));

	TInt r;

	// Switch over the request type. Typically the requests that are
	// passed here will be RExdDriver::TControl enumerated requests
	// sent by the user in user side DoControl() request
	//
	switch(aFunction)
		{
		// User request to read the channel capabilities of Uart driver
		//
		case RExDriverChannel::EControlCaps:
			
			// Call Caps() to retrieve capabilities to a1
			r = Caps((TDes8*)a1);
			
			break;
		// User request to configure the device (uart)
		//
		case RExDriverChannel::EControlSetConfig:
			// Call SetConfig function by passing the first argument
			r = SetConfig((const TDesC8*)a1);
			break;
		// User request to enable or disable the device's internal loopback mode
		//
		case RExDriverChannel::EControlSetIntLoopback:
			// Call SetIntLoopback function by passing the first argument
			r = SetIntLoopback((const TInt*)a1);
			break;
		case RExDriverChannel::ERequestTransmitData:
			// Call TransmitData function
			
			// Check if already transmit is going on this channel by some other
			// user thread. If yes then return KErrInUse.
			WaitOnTxFMutex();
			if (iTxProgress)
				{
				SignalTxFMutex();
				return KErrInUse;
				}
			
			// Mark Transmit Data progress flag as true.
			iTxProgress = ETrue;
			SignalTxFMutex();
			r = TransmitData((const TDesC8*)a1);
			break;

		case RExDriverChannel::ERequestReceiveData:
			// Call receive data function
			
			// Check if already receive data is going on this channel by some other
			// user thread. If yes then return KErrInUse.
			WaitOnRxFMutex();
			if (iRxProgress)
				{
				SignalRxFMutex();
				return KErrInUse;
				}
			
			// Mark Receive Data progress flag as true.
			iRxProgress = ETrue;
			SignalRxFMutex();
			r = ReceiveData((TDes8*)a1);
			break;
		
		default:
			// Unknown request and therefore not supported
			//
			r = KErrNotSupported;
		}
	return r;
	}

/**
 Get the channel capabilities of UART. This function reads the channels
 capabilities from the Platform Specific Layer (PSL). User uses this info
 to configure the channel as per supported capabilties.

 @param		aCaps
 			Buffer to read the channel capabilities
 @return	KErrNone on success or system wide error code on failure of
 			data copy to user
 */
 TInt DExDriverLogicalChannel::Caps(TDes8* aCaps)
 	{
 	
	
	//  Package buffer for TCommCapsV03
 	TCommCaps3 caps;

 	// Call the PSL capabilities function
	Pdd()->Caps(caps);
	
	// Copy the caps to aCaps.
    Kern::InfoCopy(*aCaps,caps);
    
    return KErrNone;
 	}

/**
 Set the internal loopback mode of the device (Uart). This is device specific API,
 that provides functionality to configure the uart's internal loopback mode, as
 set by the user.User calls this by using RBusLogicalChannel::DoControl() with
 appropriate request number

 @param		aMode
			Holds the loopback enable and disable option
			KLoopbackEnable for enable and KLoopbackDisable for disable

 @return	KErrNone or system wide standard error code
 */
 TInt DExDriverLogicalChannel::SetIntLoopback(const TInt* aMode)
 	{
 	KEXDEBUG(Kern::Printf("++DExDriverLogicalChannel::SetIntLoopback"));

 	TInt r=KErrNone;
	TInt mode;
	
	// Read the user option.	
	kumemget (&mode, aMode , sizeof(TInt));
		
	// See the Rx and Tx Status.
	// Anything in progress then do not allow changing the loopback mode.
	WaitOnConfigDMutex();
	
	WaitOnRxFMutex();
	if (iRxProgress)
		{
		SignalRxFMutex();
		SignalConfigDMutex();
		return KErrInUse;
		}
	iRxProgress = ETrue;
	SignalRxFMutex();
	
	WaitOnTxFMutex();
	if (iTxProgress)
		{
		SignalTxFMutex();
		SignalConfigDMutex();
		return KErrInUse;
		}
	iTxProgress = ETrue;
	SignalTxFMutex();
		
	if ((mode==KLoopbackEnable)||(mode==KLoopbackDisable))
			{
 			// Invoke PDD function to actually do the hardware
			// specific configuration on the device. Here the
			// buffer pointer can be passed and accessd directly
			// as both LDD and PDD are in kernel space.
			//
			Pdd()->SetIntLoopback(mode);
			}
		else
			r= KErrArgument;
	
	WaitOnRxFMutex();
	iRxProgress = EFalse;
	SignalRxFMutex();
	
	WaitOnTxFMutex();
	iTxProgress = EFalse;
	SignalTxFMutex();
	
	SignalConfigDMutex();
	// return the result
	return r;
 	}

/**
 Configure the hardware device (Uart). This is device specific API, that
 provides functionality to configure the uart. Uart configuration settings are
 passed to this function by user. User calls this by using
 RBusLogicalChannel::DoControl() with appropriate request number

 @param		aConfig
 			configuration settings for the device
 @return	KErrNone or system wide standard error code
 */
 TInt DExDriverLogicalChannel::SetConfig(const TDesC8* aConfig)
 	{
 	KEXDEBUG(Kern::Printf("++DExDriverLogicalChannel::SetConfig"));

 	TInt r;

 	// TCommConfigV01 structure defined in d32comm.h is used
 	// to hold the configuration details like baudrate, parity,
 	// databits etc for serial
 	//
	TCommConfigV01 config;
	// Set the bytes to zero using the nanokernel
	// utility function, memclr()
	//
	memclr(&config, sizeof(config));

	// Constructs 8-bit modifiable pointer descriptor, with
	// length 0 and max length, sizeof(config)
	//
	TPtr8 cfg((TUint8*)&config, 0, sizeof(config));

	// Read the descriptor.
	//
	Kern::KUDesGet(cfg , *aConfig);
	
 	// See the Rx and Tx Status.
	// Anything in progress then do not allow setting configurations.
	WaitOnConfigDMutex();
	
	WaitOnRxFMutex();
	if (iRxProgress)
		{
		SignalRxFMutex();
		SignalConfigDMutex();
		return KErrInUse;
		}
	iRxProgress = ETrue;
	SignalRxFMutex();
	
	WaitOnTxFMutex();
	if (iTxProgress)
		{
		SignalTxFMutex();
		SignalConfigDMutex();
		return KErrInUse;
		}
	iTxProgress = ETrue;
	SignalTxFMutex();
		
 		
	// Invoke PDD function to actually do the hardware
	// specific configuration on the device. Here the
	// buffer pointer can be passed and accessed directly
	// as both LDD and PDD are in kernel space.
	//
	r = Pdd()->Configure(config);
		
	WaitOnRxFMutex();
	iRxProgress = EFalse;
	SignalRxFMutex();
	
	WaitOnTxFMutex();
	iTxProgress = EFalse;
	SignalTxFMutex();
	
	SignalConfigDMutex();	
	// return the result
	//
	return r;
 	}


/**
 Transmit the data over the serial port.Transmit data is passed
 as buffer descriptor to this function by user. User calls this
 by using RBusLogicalChannel::DoControl() to LDD and ldd inturn
 calls the PDD function to do the actual transmission to the
 device.

 @param		aData
 			Transmit buffer descriptor
 @return	KErrNone on success
 			KErrAbort on invalid length
 			or any other system wide error code
 */
 TInt DExDriverLogicalChannel::TransmitData(const TDesC8* aData)
 	{
 	KEXDEBUG(Kern::Printf("++DExDriverLogicalChannel::TransmitData"));

 	TInt r;
 	TInt maxlength,size;

  	// Reset offset for every Tx
  	iTxOffset=0;
  	
  
  	// In case of zero length buffer or invalid length, abort the request.
  	//
  	Kern::KUDesInfo(*aData,iBytesTxed,maxlength);
  	if (iBytesTxed<=0)
  		{
  		WaitOnTxFMutex();
  		iTxProgress = EFalse;
  		SignalTxFMutex();
   		return KErrAbort;	
  		}

	// Transmit data 1 frame (256 bytes) at a time.
	// Loop around till the complete User data is transferred.
	//
	do 
	{
			
 	// Copy the data ptr to intermediate ptr. 
 	//
 	iTxData=(TAny*)aData;

 	// Transmit the data to device/PDD in blocks of LDD buffer length (256).
 	// Loop around untill the complete data is transferred.
 	//
  	size=(iBytesTxed<KTxBufferSize)?iBytesTxed:KTxBufferSize;
 	
 	// Clear the buffer to zero using the nanokernel
	// utility function, memclr()
	//
	memclr(iTxBuffer, KTxBufferSize);

	// Constructs 8-bit modifiable pointer descriptor, with
	// length 0 and max length, size
	//
 	TPtr8 txbuf(iTxBuffer,0,size);

 	// Copy the descriptor from user into LDD buffer.
 	// aData->Ptr()+iTxOffset will give the pointer to the next frame to be
 	// transferred. And size is the number bytes to be copied.
 	//
 	txbuf.SetLength(size);

	kumemget(iTxBuffer,(TAny *)((aData->Ptr()) + iTxOffset),size);
				
	// Invoke PDD function to actually do the hardware specific
	// operation on the device. Here the buffer pointer can be
	// accessed directly as both LDD and PDD are in kernel space.
	//
	r = Pdd()->TransmitData(txbuf);
	
	// iBytesTxed keeps track of how many bytes transferred till now.
	iBytesTxed = iBytesTxed - size;
	// iTxOffset will give the next data frame offset.
	iTxOffset = iTxOffset + size;
		
 	} while (iBytesTxed>0);
 	
	WaitOnTxFMutex();
  	iTxProgress = EFalse;
  	SignalTxFMutex();
  	  	
	return r;
 	}


/**
 Receive data from the serial port. Data received from device is filled
 into the buffer descriptor and returned. User calls this by using
 RBusLogicalChannel::DoControl() to LDD and ldd inturn calls the PDD
 function to do the actual reception od data from the device.

 @param 	aData
 			Receive buffer descriptor, returned to caller
 			aLen
 			Length of data requested by user

  @return	KErrNone or system wide standard error code
 */
 TInt DExDriverLogicalChannel::ReceiveData(TDes8* aData)
 	{
 	KEXDEBUG(Kern::Printf("++DExDriverLogicalChannel::ReceiveData"));

 	TInt size;
 	TInt length;

 	// Reset the target for a new receive
 	iRxOffset=0;

  	// Read the descriptor info from the user thread.	
	Kern::KUDesInfo(*aData ,length ,iRxCount );
	if (iRxCount<=0)
		{
		WaitOnRxFMutex();
  		iRxProgress = EFalse;
  		SignalRxFMutex();
		return KErrAbort;
		}
	
 	// Save the user buffer pointer to copy the data received later
	iRxData = aData;

	TInt r;
	
	do 
		{
		// Handle KRxBufferSize size of data in each iteration
		size=(iRxCount<KRxBufferSize)?iRxCount:KRxBufferSize;

		// Create a descriptor of the receive buffer of requested size
		TPtr8 rxbuf((TUint8*)iRxBuffer,size);

		// Initiate receiving data and Rx interrupt notification. PSL does
		// the hardware initialization
		// Loop around receive data from the device till complete data is
		// received or any error encountered.	
		//
		TInt lengthreceived;
		r = Pdd()->InitiateReceive(rxbuf, size, lengthreceived);	
		RxDataAvailable(lengthreceived,r);
		
		} 
	while (r==KErrNone && iRxCount >0);
		
	// Flag Rx is not in progress any more.
	// 
	WaitOnRxFMutex();
  	iRxProgress = EFalse;
  	SignalRxFMutex();
	
	// Return the result of ReceieveData.
	return r;
	}


/**
 Complete the Receive function. This is called from the LDD ReceiveData function.
 This function copies the data from LDD buffer to User buffer.
 
 
 @param		aLength
 			Length of the data received
 			aResult
 			Result of the receive request on the device
 */
void DExDriverLogicalChannel::RxDataAvailable(TInt aLength, TInt aResult)
	{

	// If this function is called on KErrAbort error then do not copy data to user buffer.
	// 
	if (aResult==KErrAbort)
		{
		return;
		}

	TAny* temp;
	temp =(TAny *)((((TDes8 *)iRxData)->Ptr()) + iRxOffset);
		
	iRxCount = iRxCount - aLength;
	iRxOffset = iRxOffset + aLength;
		
	((TDes8 *)iRxData)->SetLength(iRxOffset);
	kumemput(temp,iRxBuffer,aLength);

	}

/*
 DExDriverLogicalDevice class implementation
 */

/**
 Logical device constructor. This is called while creating the LDD factory
 object as a part of the driver (LDD) loading.
 */
DExDriverLogicalDevice::DExDriverLogicalDevice()
	{
	// Set iParseMask to KDeviceAllowPhysicalDevice to tell the kernel
	// that this driver supports a PDD. Based on this flag value, kernel
	// will search for a matching PDD to load. KDeviceAllowUnit can be set
	// to make driver support multiple units. KDeviceAllowInfo can also be
	// set if driver wants to use device info.
	//
	iParseMask = KDeviceAllowPhysicalDevice | KDeviceAllowUnit;

	// LDD is not deciding which units have to be supported. PDD defines
	// which units are supported by setting iUnitsMask in physical device.
	// Hence, mask is set to 0xffffffff here.
	//
	iUnitsMask=KUartUnitMaskLdd;

	// Set the version of the interface supported by this LDD, that consists
	// of driver major number, device minor number and build version number.
	// It will normally be incremented if the interface changes.Validating
	// code assumes that clients requesting older versions will be OK with
	// a newer version, but clients requesting newer versions will not want
	// an old version.
	//
	iVersion = TVersion(KExLddMajorVerNum,KExLddMinorVerNum,KExLddBuildVerNum);
	}

/**
 Logical device destructor
 */
DExDriverLogicalDevice::~DExDriverLogicalDevice()
	{
	// If the logical device holds any resources that are allocated,
	// they can be freed here in the destructor
	}

/**
 Install the LDD. This is second stage constructor for logical device,
 called after creating LDD factory object on the kernel heap to do further
 initialization of the object.

 @return	KErrNone or standard error code
 */
TInt DExDriverLogicalDevice::Install()
	{
	// Install() should as minimum, set the driver name. Name is important
	// as it is the way in which these objects are subsequently found and
	// is property of reference counting objects. SetName() sets the name
	// of the refernce counting object created. User specifies name in
	// RBusLogicalChannel::DoCreate() and the device driver framework finds
	// the factory object by matching the name.
	//
	return SetName(&KDriverName);
	}

/**
 Get the capabilities of the device. This is called in result of
 RDevice::GetCaps. (RDevice being the user side class for LDD object).
 Device capabilities can be packaged in any form, implementation based
 and copied to the user using kernel API.

 @param	aCaps
 		descriptor returned after filling with capabilities
 */
void DExDriverLogicalDevice::GetCaps(TDes8& aCaps) const
	{
	// Package TCapsDevCommV01 object into a modifiable buffer descriptor using
	// TPckgBuf. The package provides a type safe way of transferring an
	// object or data structure which is contained within a modifiable
	// buffer descriptor. Typically, a package is used for passing data via
	// inter thread communication.
	//
	TPckgBuf<TCapsDevCommV01> caps;

	// The contained object is accessible through the package. Therefore,
	// initialize the object with device capabilities. Here, just version
	// is being provided.
	//
	caps().version=iVersion;

	// Copy device capabilities package to user thread using the kernel API.
	// Kern::InfoCopy() copies data from a source descriptor to a target
	// descriptor in a way that enables forward and backward compatibility.
	// Number of bytes copied is determined by target desriptor's length.
	//
	Kern::InfoCopy(aCaps,caps);
	}

/**
 Logical device create. This inturn creates a logical
 channel. It is called in the context of the client user-side thread
 that requested the creation of the logical channel. This is a result of
 a user-side call to RBusLogicalChannel::DoCreate().

 @param 	aChannel
 			reference to the logical channel object created

 @return	KErrNone for success or KErrNoMemory for failure
 */
TInt DExDriverLogicalDevice::Create(DLogicalChannelBase*& aChannel)
	{
	// Check if the channel is already opened. DLogicalDevice::iOpenChannels
	// has the number of DLogicalChannelBase objects currently in existence 
	// which have been created from this LDD
	//
	if (iOpenChannels!=0)	
		{
		// Here we are supporting single channel and hence channel creation
		// is not allowed if the channel is already opened. 
		//
		return KErrInUse;
		}
	
	// Create the logical channel, if there is no channel already opened.
	//
	aChannel = new DExDriverLogicalChannel;
	if (!aChannel)
		{
		// Logical Channel creation fail
		return KErrNoMemory;
		}

	// Successful logical channel creation.
	return KErrNone;
	}

//
// End of d_exsync_ldd.cpp
