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
// DExDriverLogicalDevice will be implemented in this function
// 
//

// include files
// 
#include "expio.h"		 // interface file - contants shared with user
#include "d_expio_ldd.h"

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
	// Retrieve the current thread information using the kernel API. This 
	// API gets the current Symbian OS thread, i.e refernce to DThread 
	// object.This thread information is required later in driver to 
	// perform operations like data copy etc. across different threads.
	//
	iClient=&Kern::CurrentThread();

	// Open kernel side refernce-counted object by calling DObject::Open()
	// DThread is derived from DObject. Opening this object increments the
	// refernce count by one atomically.
	//
	((DObject*)iClient)->Open();
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
										const TDesC8* anInfo, 
										const TVersion& aVer)
	{
	// not supporting units, it shall be negative as KDeviceUnit 
	// is not specified	iniParseMask
	//
	(void)aUnit; 
	(void)anInfo; // not using info
	
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
		
	// Logical channel based driver has a DFC created for itself. A dedicated
	// DFC can be created for the driver, instead of using default kernel DFC
	// thread 0. PDD implements DFCQ function and returns the newly created
	// DFCQ. All synchronous and asynchronous messages and events for this 
	// driver will be queued to this queue. SetDfcQ() initializes the message
	// queue used by LDD with the DFC(TDfcQue object) returned by Pdd()->DfcQ()
	//
	SetDfcQ(Pdd()->DfcQ());    	
 
	// Set the message queue to start receiving the messages.
	// TMessageQue::Receive() will request driver to receive next message
	// (sent by client) in the queue. It makes the queue as ready and also 
	// activates the DFC when the message is present in queue.
	//
    iMsgQ.Receive();
	
	// Allocate buffers for Tx and Rx. Kern::Alloc allocates a block of 
	// specified size and zeo fills the memosry block. On success this returns
	// pointer to the allocated buffer, else returns NULL pointer
	// 
	iTxBuffer = (TUint8*) Kern::Alloc(KTxBufferSize);
	if (!iTxBuffer)
		return KErrNoMemory;
	
	iRxBuffer = (TUint8*) Kern::Alloc(KRxBufferSize);
	if (!iRxBuffer)
		return KErrNoMemory;
	
	// return no error
	//
    return KErrNone;
	}
	
/**
 Destructor for the logical channel
 */
DExDriverLogicalChannel::~DExDriverLogicalChannel()
	{
	// If the allocated buffers for Tx and Rx are still existing delete them
	//	
	if (iTxBuffer)
		delete iTxBuffer;
	
	if (iRxBuffer)
		delete iRxBuffer;
	
	// Close the refernce on client thread using the kernel API 
	// Kern::SafeClose(). It swaps the kernel side reference counted object
	// to NULL value atomically and closes the object. This operation is 
	// done in response to DObject::Open() in Logical Channel's constructor.
	//
	Kern::SafeClose((DObject*&)iClient,NULL);
	}

/** 
 Notifies an object that a handle to it has been requested by user thread. 
 It allows the driver to control access to itself from user threads. It 
 can decide here, if it would like to allow more than one thread/process.
 
 @param 	aThread
 			thread object
 @param 	aType
 			owner, can be thread or process
 @return	KErrNone or standard error code
 */
TInt DExDriverLogicalChannel::RequestUserHandle(DThread* aThread, 
												TOwnerType aType)
    {
    // Checks if the client trying to access the driver, can be allowed to 
    // get a handle created. Here, any thread that has not created logical 
    // channel object is not allowed to create a handle to the driver.Here,
    // it supports a single client thread per driver. However, this can 
    // also be implemented to support multiple clients, if required.
    //
    if (aType!=EOwnerThread || aThread!=iClient)
        return KErrAccessDenied;
    
    return KErrNone;
    }
    
/**
 Handle the incoming client messages (synchronous, asynchronous) to the 
 driver. This is called in context of DFC thread. Received message is 
 passed as the argument, TThreadMessage object. It is upto the driver to
 interpret the request type and handle it accordingly. Driver should call
 TThreadMessage::Complete() to end to unblock the user thread and notify
 that the message is received and will be handled.
  
 @param	aMsg
 		received message
 */
void DExDriverLogicalChannel::HandleMsg(TMessageBase* aMsg)
	{
	// Retrieve the message object
	//
	TThreadMessage& m = *(TThreadMessage*)aMsg;
		
	// Get the request value
	//
	TInt id = m.iValue;
	
	// Handle logical channel close message received when client closes
	// logical channel. ECloseMsg request is issued by the framework
	// DLogicalChannel::Close(), which is called by RHandle::Close() while
	// closing the channel.
	//
	if (id==(TInt)ECloseMsg)
		{
		// Unblock the user thread and notify the completion of handling 
		// the request successfully.EFalse here sets the option of receiving
		// next kernel message to False, as we are closing down the channel.
		//
		m.Complete(KErrNone,EFalse);
		return;
		}
		
	// Check if the message is from the thread that created the logical 
	// channel. It does not support requests from any other threads.
	//
    if(m.Client()!=iClient)
        {
        // Kill the user thread. ThreadKill() kills the request sending 
        // thread with a panic. TThreadMessage::Client() returns pointer
        // to the sending thread.
        //
        Kern::ThreadKill(m.Client(),EExitPanic,ERequestFromWrongThread,
        				_L("Tutorial Driver Panic"));        
        m.Complete(KErrNone,ETrue);
        return;
        }
        
	// If the request value is 0 or positive, then the it is a synchronous 
	// request. This type of message is result of user side DoControl()
	// request with function number equal to iValue.
	if (id>=0)
		{
		// Handle the synchronous requests. This is implementation specific,
		// however as a general practice, they are handled in DoControl().
		// m.Ptr0() and m.Ptr1() gets the arguments provided with the request.
		//
		TInt r = DoControl(id,m.Ptr0(),m.Ptr1());
		
		// Unblock the user thread and notify the completion of handling 
		// the request successfully.ETrue here sets the option of receiving
		// next kernel message to true.
		//
		m.Complete(r,ETrue);
		return;
		}	
	}
	
/** 
 Handle Synchronous Requests. This is called from HandleMsg() to process
 synchrounous requests, that perform the action quickly and returns.
 Hanlding these requests is implementation specific. However, if it needs
 any hardware operation, then PDD function can be invoked from here.
 
 @param 	aFunction
 		  	Request number
 @param 	a1
 			argument1 passed by user for the request
 @param 	a2
 			argument2 passed by user for the request
 @return	KErrNone or standard error code
 */	
TInt DExDriverLogicalChannel::DoControl(TInt aFunction, TAny* a1, 
										TAny* a2)
	{
	TInt r;
				
	// switch over the request type. Typically the requests that are
	// passed here will be one on RExdDriver::TControl enumerated requests
	// sent by the user in user side DoControl() request
	//
	switch(aFunction)
		{
		// User request to read the channel capabilities of Uart driver
		//
		case RExDriverChannel::EControlCaps:		
			// Check if the client thread has ECapabilityCommDD capability
			// for the driver to process this request. Driver can enforce
			// more capability checks (other than those checked during channel
			// creation), for individual requests.
			//			
			if(iClient->HasCapability(ECapabilityCommDD,
					__PLATSEC_DIAGNOSTIC_STRING("Checked by Tutorial Driver")))
				{
				// Call Caps() to retrieve capabilities to a1
				r = Caps((TDes8*)a1);	
				}
            	else
            	{
            	// if the required capability is not present for the user
            	// thread to process this request, then deny the request.
            	//
            	r=KErrPermissionDenied;            
            	}	                
			break;
		// User request to configure the device (uart)		
		//
		case RExDriverChannel::EControlSetConfig:		
			// Check if the client thread has ECapabilityCommDD capability
			// for the driver to process this request. Driver can enforce
			// more capability checks (other than those checked during channel
			// creation), for individual requests.
			//			
			if(iClient->HasCapability(ECapabilityCommDD,
					__PLATSEC_DIAGNOSTIC_STRING("Checked by Tutorial Driver")))
				{
				// Call SetConfig function by passing the first argument
				r = SetConfig((const TDesC8*)a1);	
				}
            	else
            	{
            	// if the required capability is not present for the user
            	// thread to process this request, then deny the request.
            	//
            	r=KErrPermissionDenied;            
            	}	                
			break;
			
		// User request to transmit data to the device (uart)		
		//	
		case RExDriverChannel::EControlTransmitData:		
			// Call TransmitData function with data
			r = TransmitData((const TDesC8*)a1);
			break;	
			
		// User request to receive data from the device (uart)		
		//
		case RExDriverChannel::EControlReceiveData:											
			// Call receive data function with buffer and length											
			r = ReceiveData((TDes8*)a1,a2);							
			break;
		default:
			// Unknown request and therefore not supported
			//
			r = KErrNotSupported;			
	}
	// returns KErrNone or standard error code as returned by API used above
	//
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
 	TInt r;
 	//  Package buffer for TCommCapsV03
 	TCommCaps3 caps;
 	// Call the PSL capabilities function 	 
	Pdd()->Caps(caps);	
	// Write the descriptor to the user thread, iClient using 
	// kernel API, Kern::ThreadDesWrite to copy caps to aCaps.
	//
    r=Kern::ThreadDesWrite(iClient,aCaps,caps,0);    	
	return r;							
 	}
 		
/**
 Configure the hardware device (Uart). Uart configuration settings are 
 passed to this function by user. User calls this by using 
 RBusLogicalChannel::DoControl() to LDD and ldd inturn calls the PDD
 function to do the actual operation on the device.
  
 @param	aConfig
 		configuration settings for the device
 
 @return	KErrNone or system wide standard error code
 */
 TInt DExDriverLogicalChannel::SetConfig(const TDesC8* aConfig)
 	{ 	
 	TInt r;
 	
 	// TCommConfigV01 structure defined in d32comm.h is used
 	// to hold the configuration details like baudrate, parity,
 	// databits etc for serial
 	//	
	TCommConfigV01 config;
	
	// Set the number of bytes to zero using the nanokernel 
	// utility function, memclr()				
	//
	memclr(&config, sizeof(config));
				
	// Constructs 8-bit modifiable pointer descriptor, with 
	// length 0 and max length, sizeof(c)
	//
	TPtr8 cfg((TUint8*)&config, 0, sizeof(config));
			
	// Read the descriptor from user thread, iClient using 
	// kernel API, Kern::ThreadDesRead to copy a1 to cfg.
	// Incase of reading raw data,Kern::ThreadRawData() can 
	// be used. Then, TPtr8 descriptor need not be created.
	// Here, config is source and cfg is destination.
	//
	r = Kern::ThreadDesRead(iClient,aConfig,cfg,0,0);							
	if (r==KErrNone)
		{					
 		// Invoke PDD function to actually do the hardware 
		// specific configuration on the device. Here the 
		// buffer pointer can be passed and accessd directly 
		// as both LDD and PDD are in kernel space.
		//
		r = Pdd()->Configure(config);
		}
		
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
 @return	KErrNone or system wide error code
 */
 TInt DExDriverLogicalChannel::TransmitData(const TDesC8* aData) 	
 	{ 
 	TInt r=KErrNone;
 	TInt size;
 	TInt count;
 	TInt offset=0;
  
  	// In case of zero length buffer, abort the request
  	//
 	if (aData->Size()<=0)
 		{
 		// Abort the request in case of invalid length
 		return KErrAbort;
 		}
 		
 	count=aData->Size();
 	
 	// Loop till all requested amount of data is sent 	
 	while (count>0)
 		{
 		size=(count<KTxBufferSize)?count:KTxBufferSize;
 		
 		// Constructs 8-bit modifiable pointer descriptor, with 
		// length 0 and max length of size aData
		//
 		TPtr8 txbuf(iTxBuffer,0,size);
 	
 		// Read the descriptor from user thread, iClient using 
		// kernel API, Kern::ThreadDesRead to copy a1 to iTxBuffer.
		// Incase of reading raw data,Kern::ThreadRawRead() can 
		// be used. Here since TransmitData() is using buffer 
		// descriptors, we are using Kern::ThreadDesRead()
		// Here, aData is source and iTxBuffer is destination.
		//				
		r=Kern::ThreadDesRead(iClient,aData,txbuf,offset,KChunkShiftBy0);
		if (r!=KErrNone)
			break;
						
		// Invoke PDD function to actually do the hardware specific
		// operation on the device. Here the buffer pointer can be				
		// accessd directly as both LDD and PDD are in kernel space.				
		//
		r = Pdd()->TransmitData(txbuf);		
		if (r!=KErrNone)
			break;	
 	
 		// calculate the offset
		offset+=size;
		
		// calculate the remaining buffer size
		count-=size;			 	
 		}
	
	return r;
 	}
 	
/**
 Receive data from the serial port. Data received from device is filled 
 into the buffer descriptor and returned. User calls this by using 
 RBusLogicalChannel::DoControl() to LDD and ldd inturn calls the PDD 
 function to do the actual reception od data from the device.
 
 @param 	aData
 			Receive buffer descriptor, returned to caller
 		
  @return	KErrNone or system wide standard error code
 */
 TInt DExDriverLogicalChannel::ReceiveData(TDes8* aData,const TAny* aLen)
 	{
 	TInt r;
 	TInt size;
 	TInt count;
 	TInt offset=0; 	
 	 	  	 	
  	// Read the length as raw data from user thread, iClient using 
	// kernel API, Kern::ThreadRawRead to copy aLen to size.
	// Incase of reading descriptor,Kern::ThreadDesRead() can 
	// be used.
	//
	r = Kern::ThreadRawRead(iClient,aLen,&size,sizeof(size));		
	if ( (r!=KErrNone) || (size<=0) )
		return KErrAbort;
	
	// Copy the size of data requested	 
 	count=size;
 	
 	// Loop till the requested amount of data is received or 
 	// the data flow stops
 	//
 	while (count>0)
 		{
 		// In one go, send max KRxBufferSize of data or less
 		//
 		size=(count<KRxBufferSize)?count:KRxBufferSize;
 		
 		// Constructs 8-bit modifiable pointer descriptor, with 
		// max length of size KRxBufferSize
		// 
 		TPtr8 rxbuf(iRxBuffer,0,KRxBufferSize);
 		   		 	
		// Invoke PDD function to actually do the hardware specific
		// operation on the device. Here the buffer pointer can be				
		// accessd directly as both LDD and PDD are in kernel space.
		//	
		r = Pdd()->ReceiveData(rxbuf,size);
		if ((r!=KErrNone) && (r!=KErrTimedOut))
			break;	
						
		// Write the descriptor to user thread, iClient using 
		// kernel API, Kern::ThreadDesWrite to copy rxbuf to aData.
		// Incase of reading raw data,Kern::ThreadRawWrite() can 
		// be used. Here since RecieveData() is using buffer 
		// descriptors, we are using Kern::ThreadDesWrite(). Here,
		// aData is destination and rxbuf is source, unlike read.
		//	
		r=Kern::ThreadDesWrite(iClient,aData,rxbuf,offset);
		if (r!=KErrNone)
			return r;
		
		// No more data to read, quit
		if (rxbuf.Size()<size)
			break;
		
		// calculate the offset
		offset+=size;
			
		// calculate the remaining buffer size
		count-=size;			 	
 		}
 		
	return r;		
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
	// to make driver support multiple units (not supporting here). 
	// KDeviceAllowInfo can also be set if driver wants to use device info.
	//
	iParseMask = KDeviceAllowPhysicalDevice;

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
	// if the logical device holds any resources, they can be freed here	
	}

/**
 Install the LDD. This is second stage constructor for logical device, 
 called after creating LDD factory object on the kernel heap to do further
 initialization of the object.
 
 @return	KErrNone or standard error code
 */
TInt DExDriverLogicalDevice::Install()
	{
	// Install() should by minimum, set the driver name. Name is important 
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
	// Package TUartCaps object into a modifiable buffer descriptor using
	// TPckgBuf. The package provides a type safe way of transferring an 
	// object or data structure which is contained within a modifiable 
	// buffer descriptor. Typically, a package is used for passing data via
	// inter thread communication.
	//
	TPckgBuf<TCapsDevCommV01> caps;
		
	// The contained objects is accessible through the package. Therefore,
	// initialize the object with device capabilities. Here, just version 
	// is being provided.
	//
	caps().version=iVersion;
			
	// Copy device capabilities package to user thread using the kernel API.
	// Kern::InfoCopy() copies data from a source descriptor to a target 
	// descriptor in a way that enables forward and backward compatibility.
	// Number of bytes copied is determined by target desriptor's length.
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
// End of d_expio_ldd.cpp

