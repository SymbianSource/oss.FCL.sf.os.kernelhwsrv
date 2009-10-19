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
// DExDriverLogicalDevice will be implemented in this file. Here the
// logicalchannel provides a multithread supported implementation.
// 
//

// include files
// 
#include "exchnk.h"
#include "d_exchnk_ldd.h"

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
	//
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
        
	// Set the message queue to start receiving the messages.
	// TMessageQue::Receive() will request driver to receive next message
	// (sent by client) in the queue. It makes the queue as ready and also 
	// activates the DFC when the message is present in queue.
	//
    iMsgQ.Receive();
    
    Pdd()->SetChannelOpened(aUnit);
	// return no error
	//
    return KErrNone;
	}
	
/**
 Destructor for the logical channel
 */
DExDriverLogicalChannel::~DExDriverLogicalChannel()
	{
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
		TAny* ptr = m.Ptr0();
		
		TInt r = DoControl(id,ptr);
		
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
TInt DExDriverLogicalChannel::DoControl(TInt aFunction, TAny* a1) // a2 is not being used
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
			
		// User request to get the handle for Tx chunk. This will actually
		// create and return the handle created.	
		//
		case RExDriverChannel::EGetTxChunkHandle:
			// This is a synchronous request and a1 will hold the chunk 
			// handle value (>0 if success) after return 
			//
			r = GetTxChunkHandle((TInt*)a1);
			break;	
				
		// User request to get the handle for Rx chunk. This will actually
		// create and return the handle created.	
		//
		case RExDriverChannel::EGetRxChunkHandle:
			// This is a synchronous request and a1 will hold the chunk 
			// handle value (>0 if success) after return 
			//						
			r = GetRxChunkHandle((TInt*)a1);
			break;
			
		// User request to pass the handle for Tx chunk.
		// This shared chunk is created by other driver (Channel) and passed here.
		case RExDriverChannel::EPassTxChunkHandle:
			// This is a synchronous request and a1 will hold the chunk 
			// handle value (KErrNone if success) after return 
			//						
			r = PassTxChunkHandle((TInt*)a1);
			break;
			
		// User request to pass the handle for Rx chunk.
		// This shared chunk is created by other driver (Channel) and passed here.
		case RExDriverChannel::EPassRxChunkHandle:
			// This is a synchronous request and a1 will hold the chunk 
			// handle value (KErrNone if success) after return 
			//						
			r = PassRxChunkHandle((TInt*)a1);
			break;
						
		default:
			// Unknown request and therefore not supported
			//
			r = KErrNotSupported;			
	}
	
	return r;
	}

/**
 Get the handle to the shared transmit chunk. This function creates the chunk
 for transmit buffer and returns the handle to the user to use it. User then
 accesses the shared chunk using this handle. Handle is made for the thread
 that has made the request (need not be the thread that has opened it)
 
 @param	aHandle
 		Handle of the chunk, to be returned to user
 		
 @return KErrNone on success, standard error code on failure
 
 */
TInt DExDriverLogicalChannel::GetTxChunkHandle(TInt* aHandle)
	{
	TInt r;
	TInt hnd;
				
	// Invoke PDD function to create the chunk and return handle to the
	// user thread that made the request
	//
	hnd=Pdd()->MakeTxChunkHandle(iClient);
	if (hnd<0)
		return hnd;
			
	KEXDEBUG(Kern::Printf("Tx Chunk handle = %d",hnd));	
	
	// Write the hande value to user thread, iClient using 
	// kernel API, Kern::ThreadRawWrite to copy <hnd> to <aHandle>.
	// This request is run in user thread context. Kern::ThreadRawWrite()
	// writes the data directly and does not deal with descritors. Hence,
	// size also need to be provided to API as length info is not present as
	// in case of descriptor.
	//
	// In user context kumemget() and kumemput() API have to be used to copy
	// the data. However, these API cannot be called in critical section with
	// DMutex held. Hence, Kern::ThreadXXX API are being used
	// 
	r=Kern::ThreadRawWrite(iClient,(TAny*)aHandle,(const TAny*)&hnd,sizeof(hnd));	
	
	return r;
	}


/**
 Get the handle to the shared receive chunk. This function creates the chunk
 for receive buffer and returns the handle to the user to use it. User then
 accesses the shared chunk using this handle.
 
 @param	aHandle
 		Handle of the chunk, to be returned to user
 		
 @return KErrNone on success, standard error code on failure
 */
TInt DExDriverLogicalChannel::GetRxChunkHandle(TInt* aHandle)
	{
	TInt r;
	TInt hnd;
			
	// Invoke PDD function to create the chunk and return handle to the
	// user thread that made the request
	//				
	hnd=Pdd()->MakeRxChunkHandle(iClient);				
	if (hnd<0)
		return hnd;
	
	KEXDEBUG(Kern::Printf("Rx Chunk handle = %d",hnd));	
	
	// Write the hande value to user thread, iClient using 
	// kernel API, Kern::ThreadRawWrite to copy <hnd> to <aHandle>.
	// This request is run in user thread context. Kern::ThreadRawWrite()
	// writes the data directly and does not deal with descritors. Hence,
	// size also need to be provided to API as length info is not present as
	// in case of descriptor.
	// 
	// In user context kumemget() and kumemput() API have to be used to copy
	// the data. However, these API cannot be called in critical section with
	// DMutex held. Hence, Kern::ThreadXXX API are being used
	// 
	r=Kern::ThreadRawWrite(iClient,(TAny*)aHandle,(const TAny*)&hnd,sizeof(hnd));	
	
	
	return r;
	}

/**
 Takes the handle to the shared transmit chunk. This function takes shared chunk handle
 created by other driver (Channel) from user. PDD then opens it and populate all the physical and kernel address.
  
 @param	aHandle
 		Handle of the chunk, to be returned to user
 		
 @return KErrNone on success, standard error code on failure
 
 */
TInt DExDriverLogicalChannel::PassTxChunkHandle(TInt* aHandle)
	{
	TInt r;
	TInt hnd;
	
	
	r=Kern::ThreadRawRead(iClient,aHandle,(TAny*)&hnd,sizeof(hnd));	
	
	// Invoke PDD function to Pass the shared chunk handle to PDD
	r = Pdd()->TakeTxChunkHandle(iClient,hnd);
				
	return r;
	}


/**
 Takes the handle to the shared Receive chunk. This function takes shared chunk handle
 created by other driver (Channel) from user. PDD then opens it and populate all the physical and kernel address.
  
 @param	aHandle
 		Handle of the chunk, to be returned to user
 		
 @return KErrNone on success, standard error code on failure
 
 */
TInt DExDriverLogicalChannel::PassRxChunkHandle(TInt* aHandle)
	{
	TInt r;
	TInt hnd;
			
	r=Kern::ThreadRawRead(iClient,aHandle,(TAny*)&hnd,sizeof(hnd));	
	
	// Invoke PDD function to Pass the shared chunk handle to PDD
	r = Pdd()->TakeRxChunkHandle(iClient,hnd);
		
	return r;
	}

	
/** 
 Handle Asynchronous Requests. This is called from Request() to process
 asynchrounous requests. It is responsible for setting up the hardware to create
 an event that will complete the request at some point in future. Multiple 
 outstanding asynchronous requests can be handled and is implementation specific.
 
 @param 	aReqNo
 		  	Asynchronous Request number
 @param 	aStatus
 			TRequestStatus (32-bit length) object allocated on user stack. This
 			provides the request status to the client
 @param 	a1
 			parameter1 passed by user for the request
 @param 	a2
 			parameter2 passed by user for the request	
 @return	KErrNone or standard error code
 */	
TInt DExDriverLogicalChannel::DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2)
	{
	TInt r=KErrNone;
	
	KEXDEBUG(Kern::Printf("++DExDriverLogicalChannel::DoRequest"));
	
	switch(aReqNo)
		{
		// User request to transmit data to the device (uart)		
		//	
		case RExDriverChannel::ERequestTransmitData:				
			// Check if there is another Tx request pending and we received
			// another Tx request, then reject the request and return busy.
			//
			if (iTxDataStatus)
				{
				// return busy
				return KErrInUse;	
				}	

				// The status of transmit request status is stored to use while
				// notifying the completion of the request at a later point.
				//
				iTxDataStatus = aStatus;
				iTxInitialOffset = (TInt ) a1;
				
				if (iTxInitialOffset>=Pdd()->iTxChunk->iSize)
					{
					return KErrOverflow;
					}
				
				// Call TransmitData function, with offset 0
				r = TransmitData((TUint)a1,(TInt)a2);				
			break;
		// User request to receive data from the device (uart)		
		//
		case RExDriverChannel::ERequestReceiveData:						
			// Check if there is another Rx request pending and we received
			// another Rx request, then reject the request and return busy.
			//
			if (iRxDataStatus)
				{
				// return busy
				return KErrInUse;
				}

			// The status of receive request status is stored to use while
			// notifying the completion of the request at a later point.
			//
			iRxDataStatus = aStatus;
			iBytesRxed = (TUint)a2;
			iRxOffset = (TUint)a1;
			iRxInitialOffset = iRxOffset;
			
			if (iRxOffset >= Pdd()->iRxChunk->iSize)
				{
				return KErrOverflow;
				}
			// Call receive data function											
			ReceiveData((TUint)a1,iBytesRxed);
			break;
		default:
			// Unknown request and therefore not supported
			//
			r=KErrNotSupported;
		}
			
	// returns KErrNone or standard error code as returned by API used above
	//
	return r;
	}

/** 
 Cancel Asynchronous Requests. This is called from HandleMsg() to cancel pending
 asynchronous requests. This function determines which operation is to be cancelled
 and tidy up the resources specific to the request being cancelled, any outstanding
 DFCs, timers and signals the client that the operation is completed.
 
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
		if (iTxDataStatus)
			{			
    		// Stop the DMA for receive
			//
			Pdd()->StopDma(RExDriverChannel::ERequestTransmitData);
			
    		// Notify the Tx request client(iClient) that the request is completed and TRequestStatus
    		// object updated with the status and the completion code is provided to the
    		// client. KErrCancel indicates that the request has been cancelled. Typically,
    		// client thread waiting on User::WaitForRequest(TRequestStatus &aStatus) or using
    		// active object framework is unblocked and notified. Then the client may read the 
    		// request status from TRequestStatus object. 
    		//  
			Kern::RequestComplete(iClient,iTxDataStatus,KErrCancel);
			}
		}
		
	// Check if it is a receive data request	      
	//
    if(aMask&(1<<RExDriverChannel::ERequestReceiveData))
    	{
    	if (iRxDataStatus)
    		{
    		// Stop the DMA for receive
			//
			Pdd()->StopDma(RExDriverChannel::ERequestReceiveData);
			
    		// Notify the Rx request client (iClient) that the request is completed and TRequestStatus
    		// object updated with the status and the completion code is provided to the
    		// client. KErrCancel indicates that the request has been cancelled. Typically,
    		// client thread waiting on User::WaitForRequest(TRequestStatus &aStatus) or
    		// using active object framework is unblocked and notified. Then the client may
    		// read the request status from TRequestStatus object.
    		//    	
			Kern::RequestComplete(iClient,iRxDataStatus,KErrCancel);			
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
	// In user context kumemget() and kumemput() API have to be used to copy
	// the data. However, these API cannot be called in critical section with
	// DMutex held. Hence, Kern::ThreadXXX API are being used
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
	// In user context kumemget() and kumemput() API have to be used to copy
	// the data. However, these API cannot be called in critical section with
	// DMutex held. Hence, Kern::ThreadXXX API are being used
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
 Transmits the data, passed as parameter over UART device. It invokes the 
 PDD transmit function to perform the actual device specific operation.
 
 
 @param		aOffset
 			Offset in the shared chunk, where the data resides
 @param		aSize
 			Size of the data to be transmitted
 			
 @return	KErrNone on success or standard error code on failure			
 */
 TInt DExDriverLogicalChannel::TransmitData(TUint aOffset, TInt aSize) 	
 	{  	
 	TInt r;
 	TInt size;
 	
 	KEXDEBUG(Kern::Printf("++DExDriverLogicalChannel::TransmitData"));
 	
	if (Pdd()->iTxChunkLocal == Pdd()->ENone)	
		{
		Kern::RequestComplete(iClient,iTxDataStatus,KErrGeneral);
		return KErrGeneral;
		}
	// In case of zero length buffer or invalid length, abort the request.  	
  	//  
  	iBytesTxed = aSize;
  	if (iBytesTxed<=0)
 		return KErrAbort; 		
  	
  	
	
	// If Offset + demanded data size is > chunk size
	// then
	//		Get the available space from offset till chunk end.
	//  	If the demanded data size is greater than available space.
	//		then 
	//			Read only available space amount of data.
	if ((aOffset + aSize) >= Pdd()->iTxChunk->iSize)
		{
		TInt temp;
		temp = Pdd()->iTxChunk->iSize - aOffset;
		if (aSize >temp )
			aSize = temp;
		}
 	 		 	
 	// Transmit the data to device/PDD in blocks of LDD buffer length (256). 
 	// First transmit 1 frame here. In case of extra data it will be handled
 	// after completion of transmission of this frame.
 	// 	
 	size=(aSize<KTxDmaBufferSize)?aSize:KTxDmaBufferSize;
 	
	// Invoke PDD function to actually do the hardware specific
	// operation on the device. Here the buffer pointer can be				
	// accessed directly as both LDD and PDD are in kernel space.				
	//
	r = Pdd()->TransmitData(aOffset, size);
	
	
	return r;
 	}
 	
/**
 Initiates receiving data from the UART device. It enable the receive 
 interrupt,a way to get the data available notification from device
  
 */
 void DExDriverLogicalChannel::ReceiveData(TUint aOffset,TInt aSize)
 	{
 	KEXDEBUG(Kern::Printf("++DExDriverLogicalChannel::ReceiveData"));
 	
	
	// Initiate receiving data and Rx interrupt notification. PSL does
	// the hardware initialization
	//
	
	TInt temp;
	
	// If Offset + demanded data size is > chunk size
	// then
	//		Get the available space from offset till chunk end.
	//  	If the demanded data size is greater than available space.
	//		then 
	//			Read only available space amount of data.
	if ((aOffset + aSize) >= Pdd()->iRxChunk->iSize)
		{
		temp = Pdd()->iRxChunk->iSize - aOffset;
		if (aSize >temp )
			aSize = temp;
		}
			
	if (Pdd()->iRxChunkLocal == Pdd()->ENone)	
		{
		Kern::RequestComplete(iClient,iRxDataStatus,KErrGeneral);
		return;
		}
	// Receive the data from device/PDD in blocks of LDD buffer length (256). 
 	// First Receive 1 frame here. In case of extra data it will be handled
 	// after completion of Receipt of this frame.
	if (aSize > KRxDmaBufferSize)
		aSize = KRxDmaBufferSize;
		
	Pdd()->InitiateReceive(aOffset,aSize);
	return;
	}
/**
  DMA complete notification handler function. This function will be called
  by the PDD from the DMA Rx service function on successful DMA Rx operation.
  It notifies the user about receive request completion.
  
  @param	aSize
  			Accumulated size of data received over all individual reads.
  			aResult
  			DMA operation return value.
*/
 void DExDriverLogicalChannel::RxDmaComplete(TInt aSize, TInt aResult)
 	{
 	
 	KEXDEBUG(Kern::Printf("++DExDriverLogicalChannel::RxDmaComplete")); 	
 	
 	
	// Action required only if a Rx request is issued and is pending
	//
	if (iRxDataStatus)	
		{
		// If this function is called on error condition, fail the user request
		if (aResult==KErrGeneral)
			{
			// Complete the current request with the abort result.
			Kern::RequestComplete(iClient,iRxDataStatus,aResult);
			return;
			}
						
		// aSize will have the number of bytes recieved in the transfer.
	 	iBytesRxed = iBytesRxed-aSize;
	 	
	 	// Calculate the offset till where the data is written.
		iRxOffset+=aSize;
		
		if (iBytesRxed<=0)
				{
				Kern::RequestComplete(iClient,iRxDataStatus,KErrNone);	
				return;
				}
				
		// If the DFC is scheduled on a Rx timeout condition, then
		// complete the request with timeout result
		//
		if (aResult==KErrTimedOut)
			{	
			
				// Complete the current request with the timeout result.
				Kern::RequestComplete(iClient,iRxDataStatus,aResult);
			return;
			}
	 	
	 	
	 	// If we have reached the end of the chunk.
	 	// then wrap around to the initial offset position. (User specified Offset)
	 	if (iRxOffset == Pdd()->iRxChunk->iSize)		
	 		{
	 		iRxOffset =  iRxInitialOffset;
	 		}
		
		// Re-start receiving the data
		ReceiveData(iRxOffset,iBytesRxed);
		
		
		return;		
		}
 	}


/**

 Complete the transmit function. This is called from the transmit DFC handler function.
 This fucntion does any required processing for completing the transmit function and 
 then notifies the client thread about the completion using Kern::RequestComplete()
 
 @param		aResult
  			DMA operation return value.
 			aCount
  			Accumulated size of data received over all individual trasfers.
  			
  			
 */	
void DExDriverLogicalChannel::TxDataComplete(TInt aResult,TInt aCount)
	{
	TInt r;
	TInt remBytes;
		
	// Action required only if a Tx request is issued and is pending
	//
	if (iTxDataStatus) 
		{
		// If this function is called on error condition, then fail the request
		if (aResult!=KErrNone)
			{		
			Kern::RequestComplete(iClient,iTxDataStatus,aResult);
			return;
			}
			
		// Calculate the offset till where the data is written
		// aCount will have the total number of bytes transferred from the beginning
		// of the Tx operation. i.e. aCount represents the accumulated size
		// transmited data over all individual transfer, and not just the number from 
		// the most recent DMA transfer.
		
		iTxOffset=aCount;
		
		// Calculate the remaining bytes of data to transmit
		remBytes=iBytesTxed-(aCount-iTxInitialOffset);
					
		// Check if there is any more data to be transmitted. If yes, send the 
		// remaining data, else notify the transmit request completion to user
		//
		if (remBytes>0)
			{
			
			// If we have reached the end of the chunk.
	 		// then wrap around to the initial offset position. (User specified Offset)
	 		if (iTxOffset == Pdd()->iTxChunk->iSize)		
		 		{
		 		iBytesTxed = remBytes;
		 		iTxOffset =  iTxInitialOffset;
	 			}
			
			// Find out how many bytes needs to be transferred , and how many bytes actually we have.
			if ((iTxOffset + remBytes) >= Pdd()->iTxChunk->iSize)
				{
				TInt temp;
				temp = Pdd()->iTxChunk->iSize - iTxOffset;
				if (remBytes >temp )
					remBytes = temp;
				}
			
	 		TInt size=(remBytes<KTxDmaBufferSize)?remBytes:KTxDmaBufferSize;
	 		 					
			// Invoke PDD function to actually do the hardware specific
			// operation on the device.
			//
			r = Pdd()->TransmitData(iTxOffset,size);		
			if (r!=KErrNone)
				{
				// Fail the user request, if reading remaining data fails.
				// Kern::RequestComplete() uses the result, here (r) being passed
				// to make the request success or failure
				//			
				Kern::RequestComplete(iClient,iTxDataStatus,r);
				}
			}
		else // else condition - All data sent by user is transmitted 
			{		
			// Notify the client (iClient) that the request is completed and TRequestStatus
	    	// object updated with the status and the completion code is provided to the
	    	// client. Typically, client thread waiting on 
	    	// User::WaitForRequest(TRequestStatus &aStatus) or using active object framework
	    	// is unblocked and notified. Then the client may read the request status from 
	    	// TRequestStatus object.
	    	// 
			Kern::RequestComplete(iClient,iTxDataStatus,KErrNone);
			}
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

//
// End of d_exchnk_ldd.cpp

