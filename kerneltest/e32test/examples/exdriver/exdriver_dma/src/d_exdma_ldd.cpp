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
#include "exdma.h"
#include "d_exdma_ldd.h"

/**
 LDD entry point
 This function is called when the LDD is loaded. This creates a factory
 object for LDD. DECLARE_STANDARD_LDD macro defines the first export to
 represent the DLL factory function that will be called first by loader
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
	// API gets the current Symbian OS thread, i.e reference to DThread 
	// object.This thread information is required later in driver to 
	// perform operations like data copy etc. across different threads.
	//
	iClient=&Kern::CurrentThread();

	// Open kernel side refernce-counted object by calling DObject::Open()
	// DThread is derived from DObject. Opening this object increments the
	// reference count by one atomically.
	//
	((DObject*)iClient)->Open();
	}
	
/**
 Logical Channel - DoCreate(), called by framework after creating logical
 channel object as a part of the driver loading process. Driver should do 
 the required validation like capabilities check, version check etc. It
 associates the DFC queue which it shall use to queue the requests. It 
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
		
	// Logical channel based driver has a DFC created for itself. A dedicated
	// DFC can be created for the driver, instead of using default kernel DFC
	// thread 0. PDD implements DFCQ function and returns the DFCQ created
	// by PDD. All synchronous and asynchronous messages and events for this 
	// driver will be queued to this queue.
	// 
	// Here, LDD leaves it to PDD to associate a context,(dfcq+kernel thread)
	// with a hardware unit, so that requests on different channels (units)
	// execute in different contexts.
	//
	// SetDfcQ() initializes the message queue used by LDD with the 
	// DFC(TDynamicDfcQue object) returned by Pdd()->DfcQ().
	//
    SetDfcQ(Pdd()->DfcQ(aUnit));
    
    Pdd()->SetChannelOpened(aUnit);
    
	// Set the message queue to start receiving the messages.
	// TMessageQue::Receive() will request driver to receive next message
	// (sent by client) in the queue. It makes the queue as ready and also 
	// activates the DFC when the message is present in queue.
	//
    iMsgQ.Receive();
	
	// Create queues (derived from SDbleQue) for Tx and Rx requests to support 
	// multiple outstanding similar asynchrnous requests. SDblQue is an anchor 
	// for the doubly linked list of SDblQueLink items (i.e TExReq here). 
	//	
	iTxReqQ=new TExReqQue;
	if (!iTxReqQ)
		return KErrNoMemory;
	
	iRxReqQ=new TExReqQue;
	if (!iRxReqQ)
		return KErrNoMemory;
	
	// Pre-allocate a fixed number of requests to avoid memory bound operations
	// if done at the time of issuing a request. Use the requests from the pools
	// later when required and return to free pool when not used.
	//
	iTxReqQ->Create(); // Create Tx request pool
	iRxReqQ->Create(); // Create Rx request pool
	
	// return no error
    return KErrNone;
	}
	
/**
 Destructor for the logical channel
 */
DExDriverLogicalChannel::~DExDriverLogicalChannel()
	{
	// Delete the request queues that are created at the time of opening
	// a channel. This should free the memory allocated for the pools
	//
	if (iTxReqQ)
		delete iTxReqQ;
	if (iRxReqQ)
		delete iRxReqQ;
	
	// Close the reference on client thread using the kernel API 
	// Kern::SafeClose(). It swaps the kernel side reference counted object
	// to NULL value atomically and closes the object. This operation is 
	// done in response to DObject::Close() in Logical Channel's destructor.
	//
	Kern::SafeClose((DObject*&)iClient,NULL);
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
    // Checks if the client trying to access the driver, can be allowed to 
    // get a handle created. Here, any thread that has not created logical 
    // channel object is not allowed to create a handle to the driver.Here,
    // it supports a single client thread per driver. However, this can 
    // also be implemented to support multiple clients, as required by design.
    //
    if (aType!=EOwnerThread || aThread!=iClient)
        return KErrAccessDenied;
    
    return KErrNone;
    }
    
/**
 Handle the incoming client messages (synchronous, asynchronous) of the 
 driver. This is called in context of DFC thread. Received message is 
 passed as the argument, TThreadMessage object. It is upto the driver to
 interpret the request type and handle it accordingly. Driver should call
 TThreadMessage::Complete() to end, to unblock the user thread and notify
 that the message is received and will be handled.
  
 @param	aMsg
 		received message
 */
void DExDriverLogicalChannel::HandleMsg(TMessageBase* aMsg)
	{			
	// Retrieve the message object	
	TThreadMessage& m = *(TThreadMessage*)aMsg;
		
	// Get the request value	
	TInt id = m.iValue;
	
	// Handle logical channel close message called when client closes logical
	// channel. ECloseMsg request is issued by the framework DLogicalChannel::Close(), 
	// which is called when closing channel by RHandle::Close()
	//
	if (id==(TInt)ECloseMsg)
		{
		// DoCancel() will process the cancel request, by cancelling any 
		// pending asynchronous requests.
		//
		DoCancel(RExDriverChannel::EAllRequests);
		
		// TMessageBase::Complete() completes the kernel message and optionally
        // receives the next one. KErrNone specifies successful completion of message
        // and EFalse specifies not allowing receiving next message in the queue.
        // Unblocks the client thread that made the request.
        //	
		m.Complete(KErrNone,EFalse);
		return;
		}	
		
	// Check if the message is from the thread that created the logical 
	// channel. It does not support requests from any other threads.
	//
    if(m.Client()!=iClient)
        {
        // TMessageBase::PanicClient() panics the sender of a kernel message.
        // It also completes the message with reason code KErrDied, so that the
        // open reference on the client can be closed. This API will take care
        // of killing the thread and completing the message,(equivalent to what is
        // done in d_expio_ldd.cpp)
        //
        aMsg->PanicClient(_L("Tutorial Driver Panic"),ERequestFromWrongThread);        
        return;
        }
        
	// If client cancels asynchronous operation, the framework will issue a 
	// request with a special value of KMaxTInt(0x7FFFFFFF).
	//
    if (id==KMaxTInt)
        {
        // Handle the cancel operation from client by cancelling the specified 
        // operation. Operation is specified in the first message parameter.  
        //
        DoCancel(m.Int0());        
        // TMessageBase::Complete() completes the kernel message and optionally
        // receives the next one. KErrNone specifies successful completion of message
        // and ETrue specifies allowing receiving next message in the queue. Unblocks
        // the client thread that made the request.
        //
        m.Complete(KErrNone,ETrue);
        return;
        }
        
    // If the request value is negative, then it is an asynchronous request.
    // This type of message is a result of user side DoRequest() request with 
    // function number equal to iValue. 
    //    
    if (id<0)
    	{
    	// Message first parameter will hold the TRequestStatus object. TRequestStatus
    	// is used to hold the completion status of the request. This is updated by driver
    	// and it is checked by the client after request complete notification. 
    	//
    	// Since TRequestStatus from the client's address space is not read into this thread
    	// address space (The Kernel address space) driver cannot operate on it in any way
    	// other than for completing the request.(Kern::RequestComplete will use inter 
    	// thread APIs to write to client address space). Otherwise we would have to create
    	// a local copy and read the request status from client address space.
    	// 
    	TRequestStatus* pS = (TRequestStatus*)m.Ptr0();
    	
    	// DoRequest() handles asynch requests with ~id giving async request value
    	TInt r = DoRequest(~id, pS, m.Ptr1(), m.Ptr2());
    	if (r!=KErrNone)
    		{
    		// Notify the client (iClient) that the request is completed and 
    		// TRequestStatus object is updated with the status and the completion
    		// code. At this point, client thread is blocked and is waiting on message
    		// queue semaphore till message complete notification.
    		//    
    		Kern::RequestComplete(iClient,pS,r);
    		}
    		
    	// Complete the message	successfully and enable receiving next message    	
    	m.Complete(KErrNone,ETrue);
    	}
        
	// If the request value is 0 or positive, then it is a synchronous 
	// request. This type of message is result of user side DoControl()
	// request with function number equal to iValue.
	//
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
		}	
	}
	
/** 
 Handle Synchronous Requests. This is called from HandleMsg() to process
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
			// Check if the client thread has ECapabilityCommDD capability
			// for the driver to process this request. Driver can enforce
			// more capability checks (other than those checked during channel
			// creation), for individual requests.
			//
			// Any API specific capability can be checked this way. Capability
			// that has already been checked while opening need not be checked
			// again here.
			//			
			if(iClient->HasCapability(ECapabilityCommDD,
					__PLATSEC_DIAGNOSTIC_STRING("Checked by Tutorial Driver")))
				{
				// Call Caps() to retrieve capabilities to a1
				r = Caps((TDes8*)a1);	
				}
            else
            	{
            	// If the required capability is not present for the user
            	// thread to process this request, then deny the request.
            	//
            	r=KErrPermissionDenied;            
            	}	                
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
		default:
			// Unknown request and therefore not supported
			//
			r = KErrNotSupported;			
		}
	return r;
	}

/** 
 Handle Asynchronous Requests. This is called from HandleMsg() to process
 asynchrounous requests. It is responsible for setting up the hardware to create
 an event that will complete the request at some point in future. Multiple 
 outstanding asynchronous requests can be handled and is implementation specific.
 Here, multiple outstanding requests (multiple Tx or multiple Rx) are queued and
 handled in the order. It also supports full duplex operation, i.e Tx and Rx 
 concurrently.
 
 @param 	aReqNo
 		  	Asynchronous Request number
 @param 	aStatus
 			TRequestStatus (32-bit length) object allocated on user stack. This
 			provides the request status to the client
 @param 	a1
 			parameter1 passed by user for the request
 @param 	a2
 			parameter2 passed by user for the request (not used here)	
 @return	KErrNone on success 
 			or error codes (KErrNoMemory, KErrAbort, KErrNotSupported, KErrTimedOut)
 */	
TInt DExDriverLogicalChannel::DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2)
	{
	TInt r;
	TExReq *req;
	
	KEXDEBUG(Kern::Printf("++DExDriverLogicalChannel::DoRequest"));
	
	switch(aReqNo)
		{
		// User request to transmit data to the device (uart)		
		//	
		case RExDriverChannel::ERequestTransmitData:
			// Acuire a free object from the Tx pool
			req=(TExReq *)iTxReqQ->NextFreeRequest();
			if (!req)
				{
				// No free request available in the Tx pool
				return(KErrNoMemory);
				}
				
			// Save the request related info, i,e status and data ptr 
			// before queuing the request
			//
			req->iRequest=aStatus;
			req->iData=a1;
			
			// Add the new request to the pending Tx queue
			iTxReqQ->AddToPendingQ(req);					
			
			// if no request is pending, then handle it now 
			if (!iTxReqCount)
				{
				// Increment the request count
				iTxReqCount++;				
				// Get the current request status object		
				iTxDataStatus=req->iRequest;
				// Start transmitting data
				r=TransmitData((const TDesC8*)(req->iData));
				}
			else
				{
				// Requests are already pending, hence increment and
				// return. Note, request is already queued
				//
				iTxReqCount++;
				r=KErrNone;
				}
			break;
		// User request to receive data from the device (uart)		
		//
		case RExDriverChannel::ERequestReceiveData:						
			// Acuire a free object from the Rx pool
			req=(TExReq *)iRxReqQ->NextFreeRequest();
			if (!req)
				{
				// No free request available in the Rx pool
				return(KErrNoMemory);
				}
				
			// Save the request related info, i,e status and data ptr 
			// before queuing the request
			//	
			req->iRequest=aStatus;
			req->iData=a1;
			// Add the new request to the pending Rx queue
			iRxReqQ->AddToPendingQ(req);					
			
			// if no request is pending, then handle it now 
			if (!iRxReqCount)
				{
				// Increment the request count
				iRxReqCount++;
				// Get the current request status object		
				iRxDataStatus=req->iRequest;
				// Start transmitting data
				
				r=ReceiveData((TDes8*)(req->iData));
				}
			else
				{
				// Requests are already pending, hence increment and
				// return. Note, request is already queued
				//
				iRxReqCount++;
				r=KErrNone;				
				}
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
 Cancel Asynchronous Requests. This is called from HandleMsg() to cancel pending
 asynchronous requests. This function determines which operation is to be cancelled
 and tidy up the resources specific to the request being cancelled, any outstanding
 DFCs, timers and signals the client that the operation is completed. However, it
 cancels all the outstanding requests of same type, e.g if a cancel is called on
 transmit request, then all outstansding transmit requests are cancelled, same with
 
 @param 	aMask
 		  	Mask containing the request number that has to be cancelled 
 */	
void DExDriverLogicalChannel::DoCancel(TUint aMask)
	{	
	KEXDEBUG(Kern::Printf("++DExDriverLogicalChannel::DoCancel"));
	
	// Any of the asynchronous operation if pending can be cancelled.Hence check if
	// it is a valid asynchronous request to cancel. e.g Transmit request
	//
	if(aMask&(1<<RExDriverChannel::ERequestTransmitData))
		{		
		TExReq* req;
		
		// Stop the DMA for on transmit channel				
		Pdd()->StopDma(RExDriverChannel::ERequestTransmitData);
					
		// Complete all the requests in the queue. Traverse till the end and
		// cancel each pending request
		//
		while ((req=((TExReq*)(iTxReqQ->iUseQ.GetFirst())))!=NULL)
			{
			// Decrement the count of pending request, as it is completed
			iTxReqCount--;
			
			// Notify the client (iClient) that the request is completed and TRequestStatus
    		// object updated with the status and the completion code is provided to the
    		// client. KErrCancel indicates that the request has been cancelled. Typically, 
    		// client thread waiting on User::WaitForRequest(TRequestStatus &aStatus) or using
    		// active object framework is unblocked and notified. Then the client may read the 
    		// request status from TRequestStatus object. 
    		//  		
			Kern::RequestComplete(iClient,req->iRequest,KErrCancel);			
			
			// Return the cancelled request to the Tx free pool
			iTxReqQ->iFreeQ.Add(req);	
			}	
		}
	// Check if it is a receive data request	      
	//
    if(aMask&(1<<RExDriverChannel::ERequestReceiveData))
    	{    	
    	TExReq* req;
    	
    	// Stop the DMA for on receive channel
    	Pdd()->StopDma(RExDriverChannel::ERequestReceiveData);
    	
    	// Complete all the requests in the queue. Traverse till the end and
		// cancel each pending request
		//
		while ((req=((TExReq*)(iRxReqQ->iUseQ.GetFirst())))!=NULL)
			{
			// Decrement the count of pending request, as it is completed
			iRxReqCount--;
						
			// Notify the client (iClient) that the request is completed and TRequestStatus
    		// object updated with the status and the completion code is provided to the
    		// client. KErrCancel indicates that the request has been cancelled. Typically, 
    		// client thread waiting on User::WaitForRequest(TRequestStatus &aStatus) or using
    		// active object framework is unblocked and notified. Then the client may read the 
    		// request status from TRequestStatus object. 
    		//  		
			Kern::RequestComplete(iClient,req->iRequest,KErrCancel);			
			
			// Return the cancelled request to the Rx free pool
			iRxReqQ->iFreeQ.Add(req);	
			}    
    	}
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
	
	// Write the descriptor to user thread, iClient using 
	// kernel API, Kern::ThreadDesWrite to copy caps to aCaps.
	//
    r=Kern::ThreadDesWrite(iClient,aCaps,caps,0);    	
	return r;							
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
 		
 	TInt r;  	
	TInt mode;
				
	// Read the user option from user thread, iClient using 
	// kernel API, Kern::ThreadRawRead to copy aMode to mode.
	// Kern::ThreadRawData() reads the data from user thread
	// to the kernel thread.
	//
	r = Kern::ThreadRawRead(iClient,aMode,&mode,sizeof(TInt));
	if (r==KErrNone)
		{
		// Check if the mode requested to set is valid. It can
		// only be either enable or disable.
		//
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
			return KErrArgument;
		}
		
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
			
	// Read the descriptor from user thread, iClient using 
	// kernel API, Kern::ThreadDesRead to copy aConfig to cfg.
	// Incase of reading raw data,Kern::ThreadRawData() can 
	// be used. Then, TPtr8 descriptor need not be created.
	// Here, aConfig is source and cfg is destination.
	//
	r = Kern::ThreadDesRead(iClient,aConfig,cfg,0,0);							
	if (r==KErrNone)
		{					
 		// Invoke PDD function to actually do the hardware 
		// specific configuration on the device. Here the 
		// buffer pointer can be passed and accessed directly 
		// as both LDD and PDD are in kernel space.
		//
		r = Pdd()->Configure(config);
		}
		
	// return the result	
	return r;							
 	}

/**
 Transmits the data, passed as parameter over UART device. It reads 
 the descriptor from user to kernel space to be used further in the 
 driver and then invokes PDD to perform the actual device specific 
 operation.
 
 aData is constant here as this is intended to be non-modifiable
 
 @param		aData
 			Descriptor containing data to be transmitted 
 @return	KErrNone on success
 			KErrAbort on invalid length
 			or any other system wide error code on failure
 */
 TInt DExDriverLogicalChannel::TransmitData(const TDesC8* aData) 	
 	{ 
 	KEXDEBUG(Kern::Printf("++DExDriverLogicalChannel::TransmitData"));
 	
 	TInt r;
 	TInt size;
  
  	// Reset offset for every Tx
  	iTxOffset=0;
  	
  	// In case of zero length buffer or invalid length, abort the request.  	
  	//
  	// Kern::ThreadGetDesLength() gets the length of a descriptor in 
  	// another thread's address space. The size of the descriptor can not
  	// be checked directly by calling TDesC8::Size() as it is not safe, 
  	// and the client address space may have been moved (in the moving model). 
  	//
  	iBytesTxed = Kern::ThreadGetDesLength(iClient,aData);
  	if (iBytesTxed<=0)
 		return KErrAbort; 		
 	
 	// Copy the data ptr to intermediate ptr
 	iTxData=(TAny*)aData->Ptr();
 		 	
 	// Transmit the data to device/PDD in blocks of LDD buffer length (256). 
 	// First transmit 1 frame here. In case of extra data it will be handled
 	// after completion of transmission of this frame.
 	// 	
 	size=(iBytesTxed<KTxDmaBufferSize)?iBytesTxed:KTxDmaBufferSize;
 		
 	// Read the data from user thread, iClient using kernel API, 
 	// Kern::ThreadRawRead to copy iTxData to transmit chunk.
	// Incase of reading descriptor data,Kern::ThreadDesRead() can 
	// be used.
	//
	r=Kern::ThreadRawRead(iClient,iTxData,
						(TAny*)(Pdd()->iTxChunk->LinearAddress()),size);
	if (r!=KErrNone)
		return r;
	
	// Invoke PDD function to actually do the hardware specific
	// operation on the device. Here the buffer pointer can be				
	// accessed directly as both LDD and PDD are in kernel space.				
	//
	r = Pdd()->TransmitData(size);		
	if (r!=KErrNone)
		return r;
 	
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
 TInt DExDriverLogicalChannel::ReceiveData(TDes8* aData)
 	{
 	KEXDEBUG(Kern::Printf("++DExDriverLogicalChannel::ReceiveData"));
 	 
  	// Read the descriptor max length from the user thread, iClient using
  	// kernel API Kern::ThreadGetDesMaxLength(). This API gets the maximum
  	// length of a descriptor in another thread's address space. 
  	// Kern::ThreadGetDesInfo() can also be used to get length, maximum 
  	// length and pointer to the descriptor's buffer
	//
	iBytesRcvd=Kern::ThreadGetDesMaxLength(iClient,aData);
	
		
	if (iBytesRcvd<=0) 
		return KErrAbort;
	 	
 	// Save the user buffer pointer to copy the data received later
	iRxData = aData;
	
	// Initiate receiving data and enable Rx DMA path. PSL does
	// the hardware initialization
	//
	Pdd()->InitiateReceive();

	return KErrNone;
 	}
	
 /**
  DMA complete notification handler function. This function will be called
  by the PDD from the DMA Rx service function on successful DMA Rx operation.
  This function copies the receive data to the user buffer and notifies the
  user about receive request completion.
  
  @param	aAddr
  			Linear address of the receive chunk to which data is DMA`ed  			
  			aSize
  			Bytes received from the device by DMA transfer
  			aResult
  			Result of the DMA transfer
  */
 void DExDriverLogicalChannel::RxDmaComplete(TUint8* aAddr, TUint aSize, 
 															TInt aResult)
 	{
 	TInt r;
 	TInt size;
 	
 	KEXDEBUG(Kern::Printf("++DExDriverLogicalChannel::RxDmaComplete")); 	
 	     
	// User buffer is available only when a Rx request is pending, hence
	// copy the data to user buffer, only if a Rx request is issued and
	// is pending
	//
	if (iRxDataStatus)	
		{
		// If this function is called on error condition, fail the user request
		if (aResult==KErrGeneral)
			{
			// Complete the current pending request with result and take up 
			// the next request in the queue (if pending)
			//
			CompleteRequest(RExDriverChannel::ERequestReceiveData,aResult);
			return;
			}
						
			
		// Ensure we are reading correct amount of data, else it may result
		// in overflow during copy
		//		
		size=(aSize+iRxOffset)>iBytesRcvd?(iBytesRcvd-iRxOffset):aSize;
			
		// Constructs 8-bit modifiable pointer descriptor, with length size
	 	TPtr8 rxbuf(aAddr,size);
	 	
	 	// Set the buffer pointer of the descriptor to the specified address.
	 	// This points the Tx chunk linear address to the descriptor data pointer.
	 	//
    	rxbuf.Set(aAddr,size,size);
                
 		// Write the descriptor to user thread, iClient using 
		// kernel API, Kern::ThreadDesWrite to copy rxbuf to iRxData.
		// Incase of reading raw data,Kern::ThreadRawWrite() can 
		// be used. Here since RecieveData() is using buffer 
		// descriptors, we are using Kern::ThreadDesWrite(). Here,
		// iRxData is destination and rxbuf is source, unlike read.
		//	
		r = Kern::ThreadDesWrite(iClient,iRxData,rxbuf,iRxOffset);	
		
		// Calculate the offset till where the data is written
		iRxOffset+=size;		
		
		// If the DFC is scheduled on a Rx timeout condition, then
		// complete the request with timeout result
		//
		if (aResult==KErrTimedOut)
			{
			// Complete the current request with the timeout result. It also 
			// checks if there is any pending request in the queue and then 
			// initiates that request.
			//
			if (iRxOffset >= iBytesRcvd )
				{
				CompleteRequest(RExDriverChannel::ERequestReceiveData,KErrNone);			
				}
			else
				CompleteRequest(RExDriverChannel::ERequestReceiveData,aResult);			
			
			return;
			}
			
		if (r!=KErrNone)
			{
			// Fail the user request, if reading remaining data fails.
			// Kern::RequestComplete() uses the result, here (r) being passed
			// to make the request success or failure
			//
			CompleteRequest(RExDriverChannel::ERequestReceiveData,r);			
			return;
			}
	 	
	 			
 		// Re-start receiving the data
		Pdd()->InitiateReceive();
		
		return;	
		}
 	}
	
/* 
Complete functions called by PDD
*/

/** 
 Complete the transmit function. This is called from the transmit DMA service
 callback function in PDD. This function does any required processing for 
 completing the transmit function and then notifies the client thread about
 the completion using Kern::RequestComplete(). This completes the user request
 only if all the requested amount of data is transmitted.
 
 @param	aResult
 		Result of the DMA transfer
 		aCount
 		Number of bytes already transmitted
 */
void DExDriverLogicalChannel::TxDataComplete(TInt aResult,TInt aCount)
	{	
	TInt r;
	TInt size;
	
	// If this function is called on error condition, then fail the request
	if (aResult!=KErrNone)
		{		
		CompleteRequest(RExDriverChannel::ERequestTransmitData,aResult);
		return;
		}
			
 	// Calculate the offset till where the data is written
	iTxOffset+=aCount;
		
	// Calculate the remaining number of bytes to be transmitted
	iBytesTxed-=aCount;
		
	// Check if there is any more data to be transmitted. If yes, send the 
	// remaining data, else notify the transmit request completion to user
	//
	if (iBytesTxed>0)
		{		
		// If the remaininig data to be sent is still larger than the LDD 
		// frame size, again send in blocks of LDD frame size
		//
 		size=(iBytesTxed<KTxDmaBufferSize)?iBytesTxed:KTxDmaBufferSize;
		 		 		 		
 		// Read the descriptor from user thread, iClient using kernel API, 
 		// Kern::ThreadDesRead to copy iTxData to txbuf. Incase of reading
 		// raw data,Kern::ThreadRawRead() can be used. Here since TransmitData()
 		// is using buffer descriptors, we are using Kern::ThreadDesRead(). 
		// Here, iTxData is source and txbuf is destination. Data is copied
		// from an offset of iTxOffset in iTxData, useful in multiple iterations.
		//
		// Read the data from user thread, iClient using kernel API, 
 		// Kern::ThreadRawRead to copy iTxData to transmit chunk. Data is 
 		// copied from an offset iTxOffset in iTxData buffer. Here, API does
 		// not take care of offset as done in Kern::ThreadDesRead(). Incase 
 		// of reading descriptor data,Kern::ThreadDesRead() can be used.
		//				
		r=Kern::ThreadRawRead(iClient,((TUint8*)iTxData+iTxOffset),
						(TAny*)(Pdd()->iTxChunk->LinearAddress()),size);
		if (r!=KErrNone)
			{				
			// Fail the user request, if reading remaining data fails. This
			// completes the current request by calling Kern::RequestComplete()
			// using the result (r) passed here. It then takes up a new one,
			// if pending.
			//			
			CompleteRequest(RExDriverChannel::ERequestTransmitData, r);		
			}
				
		// Invoke PDD function to actually do the hardware specific
		// operation on the device.
		//
		r = Pdd()->TransmitData(size);		
		if (r!=KErrNone)
			{
			// Fail the user request, if reading remaining data fails. This
			// completes the current request by calling Kern::RequestComplete()
			// using the result (r) passed here. It then takes up a new one,
			// if pending.
			//			
			CompleteRequest(RExDriverChannel::ERequestTransmitData, r);		
			} 					
		}
	else // else condition - All data sent by user is transmitted 
		{
		// Notify the client (iClient) that the request is completed and TRequestStatus
    	// object updated with the status and the completion code is provided to the
    	// client. Typically, client thread waiting on 
    	// User::WaitForRequest(TRequestStatus &aStatus) or using active object framework
    	// is unblocked and notified. Then the client may read the request status from 
    	// TRequestStatus object. Then it checks any pending Tx requests from user in
    	// the queue and initiates that request
    	// 
		CompleteRequest(RExDriverChannel::ERequestTransmitData, KErrNone);
		}
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
	// Create the logical channel. Multiple channels are allowed, hence
	// no check is made on iOpenChannels
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

/*
 Complete an outstanding asynchronous request and initiate another request, 
 if pending. It handles completion of Tx and Rx requests and manages adding 
 and freeing the request queues accordingly. It also checks the queues for 
 any pending requests, and initiates them.
  
 @param		aReq
 			Request type, Transmit or Receive
 			aResult
 			Completion result of the request
 @return	none 			
 */
	
void DExDriverLogicalChannel::CompleteRequest(TInt aReq, TInt aResult)
	{
	TExReq* req;
	
	// Switch over request types		
	switch(aReq)
		{
		// Complete Tx request
		case RExDriverChannel::ERequestTransmitData:
				
			// Find the request item from the queue based on TRequestStatus object
			// The request has to be completed on this request object
			//			
			req=iTxReqQ->Find((TRequestStatus*)iTxDataStatus);
			
			// if found one, add to and free from the corresponding queues
			if (req)
				{
				// Remove the request from the pending request queue
				iTxReqQ->Remove(req);
				// Add it to the free request pool
				iTxReqQ->AddToFreeQ(req);
				// Decrement the queued pending requests
				iTxReqCount--;
				
				// Complete the request with the result. Since it is supporting
				// single client per channel, all requests are done on iClient
				//
				Kern::RequestComplete(iClient,iTxDataStatus,aResult);				
				}
			
			// Check if there is any other request pending			
			req=iTxReqQ->NextPendingRequest();	
			if (req) // if found a pending request
				{
				// Get the current request status object
				iTxDataStatus=req->iRequest;
				// Start transmitting data			
				TransmitData((const TDesC8*)(req->iData));	
				}
			break;
			
		// Complete Rx request	
		case RExDriverChannel::ERequestReceiveData:
			
			// Find the request item from the queue based on TRequestStatus object
			// The request has to be completed on this request object
			//
			req=iRxReqQ->Find((TRequestStatus*)iRxDataStatus);
			// if found one, add to and free from the corresponding queues
			if (req)
				{
				// Remove the request from the pending request queue
				iRxReqQ->Remove(req);
				// Add it to the free request pool		
				iRxReqQ->AddToFreeQ(req);
				// Decrement the queued pending requests
				iRxReqCount--;
					
				// Complete the request with the result. Since it is supporting
				// single client per channel, all requests are done on iClient
				//
				Kern::RequestComplete(iClient,iRxDataStatus,aResult);				
				}
				
			// Check if there is any other request pending
			req=iRxReqQ->NextPendingRequest();	
			if (req)  // if found a pending request
				{
				// Get the current request status object
				iRxDataStatus=req->iRequest;
				// Start receiving data
				ReceiveData((TDes8*)(req->iData));
				}
		break;
			default:
			return;			
		}
	}

/*
 Request queue implementation
 */
 
/**
 Constructor of request object queue. 
 */
TExReqQue::TExReqQue()
	{
	// Clear the request object queue memory to 0
	//
	memclr(&iReq[0],sizeof(TExReq*)*KMaxReq);
	}

/**
 Destructor of request object queue. 
 */
TExReqQue::~TExReqQue()
	{
	// Delete the requests array, frees the memory
	
	for (TInt i=0 ; i<KMaxReq; i++)
		{		
		delete iReq[i];
		}
	}

/**
 Creates the requests queue and adds the requests to the free pool.
 Second stage constructor of the request object queue
 
 @return 	KErrNone if successful, otherwise KErrNoMemory 
 */
TInt TExReqQue::Create()
	{
	TInt i;
	for (i=0;i<KMaxReq;i++)
		{
		// Create a request pool
		iReq[i]=new TExReq();
		if (!iReq[i])
			return(KErrNoMemory);		
		
		// Add the newly created request pool to the free queue
		iFreeQ.Add(iReq[i]);
		}
		
	return KErrNone;			
	}
	
/**
 Get an unused request object from the free pool
 @return  Pointer to a free request object or NULL if none are available.
 */ 	
TExReq* TExReqQue::NextFreeRequest()
	{
	// SDblQue::GetFirst() gets the first link item in the doubly linked list
	// if the list is empty, it return NULL
	//
	return((TExReq*)iFreeQ.GetFirst());
	}
	
/**
 Add a request object at the end of pending request queue. 
 @param 	aReq 
			request object pointer to be added
 */ 
void TExReqQue::AddToPendingQ(TExReq* aReq)	
	{
	// SDblQue::Add() adds the specified link item onto the end of this 
	// doubly linked list. It takes a pointer to the link item to be added
	//
	iUseQ.Add(aReq);
	}

/**
 Find a request object in pending request queue,  based on its associated 
 request status pointer.

 @param 	aStatus 
			TRequestStatus pointer of the request object to be found in the queue.
 @return  	A pointer to the request object if found or NULL if not found.
*/	
TExReq* TExReqQue::Find(TRequestStatus* aStatus)	
	{
	TExReq* retReq;
		
	// Get the first item	
	retReq=(TExReq*)iUseQ.First();	
	
	// Scan through the queue for the request holding the specified request status
	while (!IsAnchor(retReq) && retReq->iRequest!=aStatus)
		{
		// Get the next item
		retReq=(TExReq*)retReq->iNext;
		}	
	// if found, return the request object, else NULL	
	return((IsAnchor(retReq))?NULL:retReq);
	}
	
/**
 Remove a request object from the pending request queue. 
 Searches all the queue for the request and removes it
 
 @param 	aReq 
 			A pointer to the request object to be removed from the queue. 			
 @return 	A pointer to request object removed or NULL if not found.
 */	
TExReq* TExReqQue::Remove(TExReq* aReq)	
	{
	TExReq* retReq;
	
	// Get the first item	
	retReq=(TExReq*)iUseQ.First();
		
	// Scan through the queue for the request object
	while (!IsAnchor(retReq) && retReq!=aReq)
		{
		// Get the next item
		retReq=(TExReq*)retReq->iNext;
		}
			
	// If we got a match then remove the request object from 
	// pending queue
	if (!IsAnchor(retReq))
		{
		// SDblQueLink::Deque() removes this link item from the doubly 
		// linked list. Returns pointer to the link item
		retReq->Deque();
		}		
	else
		{
		// Request object not found in the queue
		retReq=NULL;
		}			
	// return the pointer if found, or NULL if not found
	return(retReq);
	}


/**
 Free up a request object by adding to the free pool  
 @param 	aReq 
 			A pointer to the freed request object
 */ 
void TExReqQue::AddToFreeQ(TExReq* aReq)	
	{
	// SDblQue::Add() adds the specified link item onto the end of this 
	// doubly linked list. It takes a pointer to the link item to be added
	//
	iFreeQ.Add(aReq);
	}


/**
 Remove the request object from the head of the queue. 
 @return 	A pointer to request object removed or NULL if the list was empty.
 */ 
TExReq* TExReqQue::Remove()
	{
	// SDblQue::GetFirst() gets the first link item in the doubly linked list
	// if the list is empty, it return NULL
	//
	return((TExReq*)iUseQ.GetFirst());
	}

/**
 Find the next request object in the pending request queue. 
 @return 	A pointer to next pending request, or NULL if none pending. 	
*/
TExReq* TExReqQue::NextPendingRequest()
	{
	TExReq* retReq;
	
	// Get the first item	
	retReq=(TExReq*)iUseQ.First();
	
	// Scan through the queue for the request object whose status is pending
	//
	while (!IsAnchor(retReq) && retReq->iRequest->Int()==KRequestPending)
		{
		// Get the next item from queue
		retReq=(TExReq*)retReq->iNext;
		}
	// return the found object or NULL if not found			
	return((IsAnchor(retReq))?NULL:retReq);	
	}
//
// End of d_exdma_ldd.cpp

