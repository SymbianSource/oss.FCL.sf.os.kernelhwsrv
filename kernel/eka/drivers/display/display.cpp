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
// os\kernelhwsrv\kernel\eka\drivers\display\display.cpp  
// LDD for a Display driver with GCE support
// 
//

/**
 @file
 @internalTechnology
 @prototype
*/


#include <drivers/display.h>
#include <kernel/kern_priv.h>


const TUint8  KMutexOrder             = 0x3f;

static const char KDisplayLddPanic[]  ="DISPLAY/GCE LDD";

_LIT(KClientRequestMutex, "ClientRequestMutex");


/************************************************************************************
 *            DDisplayLdd LDD class implementation
 ************************************************************************************/
DDisplayLdd::DDisplayLdd()		
	{	
	__DEBUG_PRINT("DDisplayLdd::DDisplayLdd()\n");
	// store the pointer to the current thread for request completion
  	iClient = &Kern::CurrentThread();
    __NK_ASSERT_DEBUG(iClient);
	// Open a reference on the client thread so it's control block can't disappear until the driver has finished with it.
    iClient->Open();
    iCurrentPostCount   = 0;
    iRequestedPostCount = 0;
    iCompositionBuffIdx = 0;
    iUnit				= -1;
    iThreadOpenCount    = 0;
    iAsyncReqCount		= 0;
    iClientRequestMutex = 0;	
	}


DDisplayLdd::~DDisplayLdd()
	{
	__DEBUG_PRINT("DDisplayLdd::~DDisplayLdd()  \n"); 
     // cancel outstanding requests and destroy the queue of TClientRequest objects
    for(TInt k = 0; k < KPendingReqArraySize ; k++)
    	{
		for(TInt i = 0; i < KMaxQueuedRequests; i++)
			{
			//Method IsReady() returns true if the client’s request SetStatus method has been called but the 
			//coresponding QueueRequestComplete method hasn't.			
			if(iPendingReq[k][i].iTClientReq)        
				{
				if(iPendingReq[k][i].iTClientReq->IsReady() )
					{
					CompleteRequest(iPendingReq[k][i].iOwningThread,iPendingReq[k][i].iTClientReq,KErrCancel);        	
					}     		
				}			
			Kern::DestroyClientRequest(iClientRequest[k][i]);
			}
		}
    
    //Close User Buffer chunks not yet destroyed.
	for(TInt i = 0; i < KDisplayUBMax; i++)
		{
		if(iUserBuffer[i].iChunk != 0)
			{
    		Kern::ChunkClose(iUserBuffer[i].iChunk);
    		iUserBuffer[i].iChunk= NULL;
			}
		}
    	
    Kern::SafeClose((DObject*&)iClient, NULL);  
    
    
    if (iClientRequestMutex != NULL)
         {
         iClientRequestMutex->Close(NULL);
         }
    
    __ASSERT_DEBUG(iThreadOpenCount == 0,Kern::Fault(KDisplayLddPanic,__LINE__));
    __ASSERT_DEBUG(iAsyncReqCount   == 0,Kern::Fault(KDisplayLddPanic,__LINE__));  
    	
    // Clear the 'units open mask' in the LDD factory.
	if (iUnit>=0)
		((DDisplayLddFactory*)iDevice)->SetUnitOpen(iUnit,EFalse);	 	

#ifdef _DEBUG
	// Close the UDEB user mode chunk, if it exists
	if (iChunk)
		Kern::ChunkClose(iChunk);
#endif // _DEBUG
	}
 

/**
	LDD second stage constructor
*/
TInt DDisplayLdd::DoCreate(TInt aUnit, const TDesC8* /* anInfo*/, const TVersion& aVer)
	{ 
	
	__DEBUG_PRINT("DDisplayLdd::DoCreate()\n"); 

	if(    !Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,      __PLATSEC_DIAGNOSTIC_STRING("Checked by DISPLAY.LDD (GCE Driver)") )
		|| !Kern::CurrentThreadHasCapability(ECapabilityReadDeviceData, __PLATSEC_DIAGNOSTIC_STRING("Checked by DISPLAY.LDD (GCE Driver)") )
		|| !Kern::CurrentThreadHasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by DISPLAY.LDD (GCE Driver)") )
		|| !Kern::CurrentThreadHasCapability(ECapabilityProtServ,       __PLATSEC_DIAGNOSTIC_STRING("Checked by DISPLAY.LDD (GCE Driver)") ) )
		{
		return KErrPermissionDenied;
		}
		  
	// Check that the display driver version specified by the client is compatible.
	if (!Kern::QueryVersionSupported(RDisplayChannel::VersionRequired(),aVer))
		{
		return(KErrNotSupported);		
		}
				
	// Check that a channel hasn't already been opened on this unit.
	TInt r=((DDisplayLddFactory*)iDevice)->SetUnitOpen(aUnit,ETrue); // Try to update 'units open mask' in the LDD factory.
	if (r!=KErrNone)
		return r;
	iUnit=aUnit;

 	Pdd()->iLdd	= this;
 	
 	r		= Pdd()->CreateChannelSetup(aUnit); 	
 	if ( r!= KErrNone)
 		{
 		return r; 		
 		}
 
     // set up user buffer nodes
    for (TInt node = 0; node < KDisplayUBMax; node++)
    	{
        iUserBuffer[node].iType 	= EBufferTypeUser;
        iUserBuffer[node].iBufferId = (node + 1);
        iUserBuffer[node].iFree 	= ETrue;
        iUserBuffer[node].iState 	= EBufferFree;
        iUserBuffer[node].iAddress  = 0;
        iUserBuffer[node].iSize  	= 0;
        iUserBuffer[node].iHandle  	= 0;
        iUserBuffer[node].iChunk  	= 0;
        iUserBuffer[node].iOffset 	= 0;
        iUserBuffer[node].iPendingRequest = 0;
    	}

    //Initialise pending queue for asynchronous requests and queue of TClientRequests
    for(TInt k = 0; k < KPendingReqArraySize; k++) 
    	{
    	
    	iPendingIndex[k]=0;
    	
		for(TInt i = 0; i < KMaxQueuedRequests; i++)
			{
			iPendingReq[k][i].iTClientReq   = 0;
			iPendingReq[k][i].iOwningThread = 0;
			
			
			r = Kern::CreateClientRequest(iClientRequest[k][i]);
			if (r != KErrNone)
				{
				return r;      	
				}
			}	
    	} 
    
    r = Kern::MutexCreate(iClientRequestMutex, KClientRequestMutex, KMutexOrder);	
	if (r != KErrNone)
		{
		return r;
		}	
    
          
    Pdd()->SetGceMode();	
	SetDfcQ(Pdd()->DfcQ(aUnit));	   
    iMsgQ.Receive();
    
    return KErrNone;		
}


/**
Override DLogicalChannel::SendMsg to process a client message before and after sending the message 
to the DFC thread for processing by HandleMsg().
 
This function is called in the context of the client thread.

The function is used to pin client data in the context of the client thread, so that data can be safely
accessed from kernel threads without the possibility of taking page faults.
 
@param aMsg  The message to process.
             The iValue member of this distinguishes the message type:
			 iValue==ECloseMsg, channel close message
			 iValue==KMaxTInt, a 'DoCancel' message
			 iValue>=0, a 'DoControl' message with function number equal to iValue
			 iValue<0, a 'DoRequest' message with function number equal to ~iValue
  
 @return KErrNone if the message was send successfully, otherwise one of the other system-wide error
        codes.
 
 */ 

TInt DDisplayLdd::SendMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt id = m.iValue;
	
	TInt r = KErrNone;	
	 // close msg or cancel
	if (id == (TInt)ECloseMsg || id == KMaxTInt)
		{
		r = DLogicalChannel::SendMsg(aMsg);
		}
	//asynchronous request
	else if (id < 0)
		{
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		r = SendRequest(aMsg);
		if (r != KErrNone)
			Kern::RequestComplete(pS,r);
		}
	 // synchronous request
	else{
		r = SendControl(aMsg);
		}
	
	return r;
	}


/**
Validate, pre-process, send and post-process data for an asynchronous client request, so that data can be safely
accessed from kernel threads without the possibility of taking page faults. 

This function is called in the context of the client thread.
 
@param aMsg  The message to process.
             The iValue member of this distinguishes the message type:
			 iValue==ECloseMsg, channel close message
			 iValue==KMaxTInt, a 'DoCancel' message
			 iValue>=0, a 'DoControl' message with function number equal to iValue
			 iValue<0, a 'DoRequest' message with function number equal to ~iValue
 
 @return KErrNone if the message was send successfully, otherwise one of the other system-wide error
        codes.
 */ 
TInt DDisplayLdd::SendRequest(TMessageBase* aMsg)
	{
	TThreadMessage& m	=*(TThreadMessage*)aMsg;
    TInt aReqNumber 	= ~m.iValue;
	TRequestStatus* pS	=(TRequestStatus*)m.Ptr0();
	
	#ifdef _GCE_DISPLAY_DEBUG
	DThread* client 	= m.Client();
	#endif		
			
	TInt r 				= KErrNotSupported;
	TInt pendingIndex;

	/*Use thread local copies of the configuration data that need to be exchanged between the client and DFC thread. 
	Using thread local copies is possible even for asynchronous requests, since the values to be returned are known 
	when processing the request( inside DoRequest ) and not at a later stage.*/
	
	TInt kernelCompBuffIdx;
	TInt kernelpack[2];	
	RDisplayChannel::TPostCount  kernelCount;
	
	
	//In asynchronous requests m.Ptr0 is used for the TRequestStatus object.
	TAny* userConfigData1 = m.Ptr1();
	TAny* userConfigData2 = m.Ptr2();
	
	/*
	If the client request needs to pass some data to the DFC thread then we copy these to a thread local
	copy(created in the thread supervisor stack) and then update the message to point to that local copy. 
	If the client will have to read a value updated in the DFC thread, then the message should just be 
	updated and point to the local copy. After the request has completed the updated data will be copied
	from the local copy to the client, in the context of the client thread. 
	*/
    
    switch (aReqNumber)
		{
	    case RDisplayChannel::EReqGetCompositionBuffer:	//Client should read data updated in the DFC thread.
				m.iArg[1] = &kernelCompBuffIdx;				
				break;

        case RDisplayChannel::EReqPostUserBuffer:    	//Both the client and DFC thread need to read data.
				umemget32(&kernelpack, userConfigData1, (sizeof(TInt)*2) );
				m.iArg[1] = &kernelpack;
				m.iArg[2] = &kernelCount;
				break;		
		
		case RDisplayChannel::EReqWaitForPost:			//Client data should be passed to the DFC thread.	
				umemget32(&kernelCount, userConfigData1, sizeof(RDisplayChannel::TPostCount) );
				m.iArg[1] = &kernelCount;							
				break;
		default:
				return KErrNotSupported; 
		
		}
		
	/* 		
	The TClientRequest objects associated with each asynchronous request need to be accessed by both the client and DFC
	threads. To resolve the potential synchronization problem we maintain two seperate pointers(iClientRequest and 
	iPendingReq.iClientReq )to the same TClientRequest object. iClientRequestMutex is used to synchronise access to 
	iClientRequest from different client threads. The first client thread that acquires the mutex will set the status
	of an available TClientRequest object and send the message(call SendMsg). Consequently method DoRequest is queued for 
	execution by the DFC thread. DoRequest initialy saves iClientRequest to iPendingReq.iClientReq and queues the request. 
	Only then, the mutex is signaled. Another client thread trying to access iClientRequest will block in the mutex until
	DoRequest has updated iPendingReq.iClientReq. Even more the DFC thread only accesses iClientRequest in DoRequest and 
	then iPendingReq.iClientReq is only used, so synchronizing access to iPendingReq.iClientReq is handled by the DFC queue.
	*/
	
	// Need to be in critical section whilst holding a DMutex
	NKern::ThreadEnterCS();
	
	Kern::MutexWait(*iClientRequestMutex);	
	//Save the TRequestStatus in any available TClientRequest object for that asynchronous request.
	
	for( TInt k=0; k< KMaxQueuedRequests; k++) 
		{
		//setStatus will return KErrInUse if a previous client request hasn't completed yet.
		r = iClientRequest[aReqNumber][k]->SetStatus(pS);
		if( r == KErrNone)
			{
			pendingIndex = k;
			//The current available index for this pending request will be passed as 
			//another message argument to the DFC thread.
			m.iArg[3] =  (TAny*) pendingIndex; 
			break;
			}		 			
		}
					 						
	if (r == KErrNone)
		{
		r = DLogicalChannel::SendMsg(aMsg);
		}
	else
		{
		__DEBUG_PRINT3("Client %08x trying to issue asynchronous request %d", client, aReqNumber);
		//Fail if there aren't any available TClientRequest object
		__ASSERT_DEBUG(r==KErrNone,Kern::Fault(KDisplayLddPanic,__LINE__));				
		}
	
	Kern::MutexSignal(*iClientRequestMutex);

    NKern::ThreadLeaveCS();
    
    //Copy config data from local copies to client, in context of client thread
    switch (aReqNumber)
		{
	    case RDisplayChannel::EReqGetCompositionBuffer:
	    		__DEBUG_PRINT2("EReqGetCompositionBuffer: kernelCompBuffIdx returned =%d",kernelCompBuffIdx);						
				umemput32(userConfigData1, &kernelCompBuffIdx, sizeof(TInt) );
				break;
				
        case RDisplayChannel::EReqPostUserBuffer:
        		__DEBUG_PRINT2("EReqPostUserBuffer: kernelCount returned = %d",kernelCount);
				umemput32(userConfigData2, &kernelCount, sizeof(RDisplayChannel::TPostCount) );	
				break;							
		}		 	
	return r;
	}


/**
Validate, pre-process, send and post-process data for a synchronous client request, so that data can be safely
accessed from kernel threads without the possibility of taking page faults. 

This function is called in the context of the client thread.

@param aMsg  The message to process.
             The iValue member of this distinguishes the message type:
			 iValue==ECloseMsg, channel close message
			 iValue==KMaxTInt, a 'DoCancel' message
			 iValue>=0, a 'DoControl' message with function number equal to iValue
			 iValue<0, a 'DoRequest' message with function number equal to ~iValue
						 
@return KErrNone if the message was send successfully, otherwise one of the other system-wide error
        codes.
 */ 

TInt DDisplayLdd::SendControl(TMessageBase* aMsg)
	{	
	TThreadMessage& m	= *(TThreadMessage*)aMsg;
    TInt aReqNumber 	= m.iValue;
	

	//Use thread local copies of the configuration data that need to be exchanged between the client and DFC thread. 
		
	RDisplayChannel::TPostCount  			kernelPostCount;
	RDisplayChannel::TDisplayRotation 		kernelRotation;
	
	TPckgBuf<RDisplayChannel::TDisplayInfo> pckgInfo(iDisplayInfo);

	TInt  kernelpack[2];	
	TInt  kernelBufferId;
	TBool kernelRotChanged;
	TInt  kernelIndex;
	
	TAny* userConfigData0 = m.Ptr0();
	TAny* userConfigData1 = m.Ptr1();
		
			
	switch (aReqNumber)
		{
		//iDisplayInfo doesn't change after the driver initialisation so copy in client thread context
		case RDisplayChannel::ECtrlGetDisplayInfo:
			umemput32(userConfigData0, &pckgInfo, sizeof(TPckgBuf<RDisplayChannel::TDisplayInfo>) );	
			return KErrNone;

		case RDisplayChannel::ECtrlPostCompositionBuffer:  		//Client should read data updated in the DFC thread.
			m.iArg[1] = &kernelPostCount;
			break;
		
		case RDisplayChannel::ECtrlPostLegacyBuffer:			//Client should read data updated in the DFC thread.
			m.iArg[1] = &kernelPostCount;			
			break;	
		
		case RDisplayChannel::ECtrlRegisterUserBuffer:			//Both the client and DFC thread need to read data.
			umemget32(&kernelpack, userConfigData0, (sizeof(TInt)*2) );
			m.iArg[0] = &kernelpack;			
			m.iArg[1] = &kernelBufferId;	
			break;
		
		case RDisplayChannel::ECtrlDeregisterUserBuffer:		//Client data should be passed to the DFC thread.	
			umemget32(&kernelBufferId, userConfigData0, sizeof(TInt) );
			m.iArg[0] = &kernelBufferId;				
			break;	
				
	 	case RDisplayChannel::ECtrlSetRotation:					//Both the client and DFC thread need to read data.
			umemget32(&kernelRotation, userConfigData0, sizeof(RDisplayChannel::TDisplayRotation) );
			m.iArg[0] = &kernelRotation;
			m.iArg[1] = &kernelRotChanged;
			break;
			
		case RDisplayChannel::ECtrlCurrentRotation:				//Client should read data updated in the DFC thread.
			m.iArg[0] = &kernelRotation;
			break;
			
		case RDisplayChannel::ECtrlGetCompositionBufferInfo:	//Both the client and DFC thread need to read data.		
			umemget32(&kernelIndex, userConfigData0, sizeof(TInt) );
			m.iArg[0] = &kernelIndex;
			m.iArg[1] = &kernelpack;		
			break;

#ifdef _DEBUG
		case RDisplayChannel::ECtrlCreateUserBuffer:
		    m.iArg[0] = userConfigData0;
            m.iArg[1] = userConfigData1;
            break;
#endif // _DEBUG

		default:
			return KErrNotSupported; 
				
		}
	
	TInt r = DLogicalChannel::SendMsg(aMsg);	
	if (r != KErrNone)
		{
		return r;
		}

	 //Copy config data from local copies to client, in context of client thread
    switch (aReqNumber)
		{					
	    case RDisplayChannel::ECtrlPostCompositionBuffer:  				
			__DEBUG_PRINT2("ECtrlPostCompositionBuffer =%d", kernelPostCount );
			umemput32(userConfigData1, &kernelPostCount, sizeof(RDisplayChannel::TPostCount) );	
			break;
		
		case RDisplayChannel::ECtrlPostLegacyBuffer:
			__DEBUG_PRINT2("ECtrlPostLegacyBuffer=%d", kernelPostCount );	
			umemput32(userConfigData1, &kernelPostCount, sizeof(RDisplayChannel::TPostCount) );			
			break;	
		
		case RDisplayChannel::ECtrlRegisterUserBuffer:
			__DEBUG_PRINT2("ECtrlRegisterUserBuffer kernelBufferId=%d", kernelBufferId );	
			umemput32(userConfigData1, &kernelBufferId, sizeof(TInt) );
			break;
		
		case RDisplayChannel::ECtrlSetRotation:
			__DEBUG_PRINT2("ECtrlSetRotation  kernelRotChanged=%d", kernelRotChanged );
			umemput32(userConfigData1, &kernelRotChanged, sizeof(TBool) );
			break;	
		
		case RDisplayChannel::ECtrlCurrentRotation:
			__DEBUG_PRINT2("ECtrlCurrentRotation kernelRotation=%d",  kernelRotation );
			umemput32(userConfigData0, &kernelRotation, sizeof(RDisplayChannel::TDisplayRotation) );
			break;
		
		case RDisplayChannel::ECtrlGetCompositionBufferInfo:				
			__DEBUG_PRINT3("ECtrlGetCompositionBufferInfo kernelpack[0] =%d and kernelpack[1] =%d",  kernelpack[0], kernelpack[1]);
			umemput32(userConfigData1, &kernelpack, (sizeof(TInt)*2) );
			break;					
		}			
	return r;
	}
	
	
/**
  All driver's client requests (synchronous and asynchronous) are sent as
  kernel messages by generic kernel to logical channel. This function
  catches messages sent by the generic kernel.
 
  @param aMsg Kernel side thread message to process.

	The iValue member of this distinguishes the message type:
	iValue==ECloseMsg, channel close message
	iValue==KMaxTInt, a 'DoCancel' message
	iValue>=0, a 'DoControl' message with function number equal to iValue
	iValue<0, a 'DoRequest' message with function number equal to ~iValue
 */ 
void DDisplayLdd::HandleMsg(TMessageBase* aMsg)
	{
    TThreadMessage& m	= *(TThreadMessage*)aMsg ;
	TInt id 			= m.iValue ;
    DThread* client 	= m.Client();

    // close message
    if (id == (TInt)ECloseMsg)
    	{
    	//Device specific cleanup operations
    	Pdd()->CloseMsg();
    	
        // cancel outstanding requests
        for(TInt k = 0; k < KPendingReqArraySize; k++)
        	{ 
			for(TInt i = 0; i < KMaxQueuedRequests; i++)
				{
	        	if( iPendingReq[k][i].iTClientReq) 
	            	{
	            	if(iPendingReq[k][i].iTClientReq->IsReady() )
	            		{
	            		CompleteRequest(iPendingReq[k][i].iOwningThread,iPendingReq[k][i].iTClientReq,KErrCancel);
	            		}            	            		
	            	}
				}
        	}
        Pdd()->SetLegacyMode();
		m.Complete(KErrNone, EFalse);
		return;
		}
    // cancel
    if (id == KMaxTInt)
		{
		// DoCancel
		TInt req = m.Int0() >> 1;
		DoCancel(req);
		m.Complete(KErrNone,ETrue);
    	return;
		}
    // asynchronous request
	else if (id < 0)
		{
		//m.Int3() is the index, in the array of pending requests, to be used.
		TInt r = DoRequest(~id,m.Ptr1(),m.Ptr2(),m.Int3(), client);
		m.Complete(r, ETrue);
		}
    // synchronous request
	else
		{
	   	TInt r = DoControl(id,m.Ptr0(),m.Ptr1(), client);
		m.Complete(r,ETrue);
		}
	} 


/**
	Cancel outstanding request.
	
	@param  aReqNumber	Any value from the RDisplayChannel::TRequest enumeration.
*/
void DDisplayLdd::DoCancel(TUint aReqNumber)
	{
      __DEBUG_PRINT2("DDisplayLdd::DoCancel %d\n",aReqNumber);

       switch (aReqNumber)
		 {
		 case RDisplayChannel::ECtrlCancelGetCompositionBuffer:
		 case RDisplayChannel::ECtrlCancelPostUserBuffer:
		 case RDisplayChannel::ECtrlCancelWaitForPost:		 
			TInt pendingIndex = iPendingIndex[aReqNumber];
	        if(iPendingReq[aReqNumber][pendingIndex].iTClientReq)
	        	{
	        	if( iPendingReq[aReqNumber][pendingIndex].iTClientReq->IsReady() )
	        		{
	        		CompleteRequest(iPendingReq[aReqNumber][pendingIndex].iOwningThread,iPendingReq[aReqNumber][pendingIndex].iTClientReq,KErrCancel);
	        		}
            	}
			break;
		}	
	
	}

/**
	Asynchronous request processing.
	
    @param aFunction    	Request function number
    @param apArg1       	Pointer to kernel message argument 1.
    @param apArg2       	Pointer to kernel message argument 2.
    @param aPendingIndex    Index pointing to the appropriate TClientRequest object to use.
	@param aClient			Pointer to the client thread that issued the asynchronous request.
	
    @return request scheduling result, system-wide error code.
*/
TInt DDisplayLdd::DoRequest(TInt aReqNumber, TAny* aArg1, TAny* aArg2, TInt aPendingIndex, DThread* aClient)
	{	

    TInt pack[2];
    TInt r 				= KErrNone;
    TBufferNode* node 	= 0;
    TInt buffer_id  	= 0;   
    
    TInt* configBufferIdx  ;
    RDisplayChannel::TPostCount* postCount;	
    TInt* configPack; 
		
	__DEBUG_PRINT5("DoRequest: iClientRequest[aReqNumber=%d][aPendingIndex=%d] is %08x  and aClient=%08x\n",aReqNumber, aPendingIndex,iClientRequest[aReqNumber][aPendingIndex], aClient);    
    
    // Open a reference on the client thread while the request is pending so it's control block can't disappear until this driver has finished with it.
	r=aClient->Open();
	__ASSERT_ALWAYS(r==KErrNone,Kern::Fault(KDisplayLddPanic,__LINE__));
#ifdef _DEBUG
	__e32_atomic_add_ord32(&iThreadOpenCount, 1);
#endif	
    	
	for(TInt i = 0; i < KMaxQueuedRequests; i++)
		{
		//Don't cancel the asyncrhonous request we currently process.
		if (i == aPendingIndex)
			{
			continue;
			}
        // cancel outstanding request
        if(iPendingReq[aReqNumber][i].iTClientReq)        
			{
			//If IsReady() returns true, setStatus has been called for that TCientRequest but QueueRequestComplete hasn't. 
			//In that case we want to cancel this outstanding request. Given that all QueueRequestComplete calls execute 
			// in the same DFC thread we currently run, there is no need to synchronise request completion calls. 
			if(iPendingReq[aReqNumber][i].iTClientReq->IsReady() )
				{
				CompleteRequest(iPendingReq[aReqNumber][i].iOwningThread,iPendingReq[aReqNumber][i].iTClientReq,KErrCancel);
				break;				
				}
        	}
    	}
	// store  index, request  and client
	iPendingIndex[aReqNumber] 								= aPendingIndex;
	iPendingReq[aReqNumber][aPendingIndex].iTClientReq 		= iClientRequest[aReqNumber][aPendingIndex];
	iPendingReq[aReqNumber][aPendingIndex].iOwningThread 	= aClient;
	
	#ifdef _DEBUG
		__e32_atomic_add_ord32(&iAsyncReqCount, 1);
	#endif   	    	    	    	    	    	  
  
    switch (aReqNumber)
		{
	     case RDisplayChannel::EReqGetCompositionBuffer:		//DFC thread updates a value the client thread should read.
			 { 
			 configBufferIdx = (TInt*) aArg1;
		 
	          TInt  index;
			  TBool found = EFalse;
			  
			  for(index = 0; index < (TInt) iDisplayInfo.iNumCompositionBuffers; index++)
					{
					if(iCompositionBuffer[index].iState == EBufferFree || iCompositionBuffer[index].iState == EBufferCompose )
						{
						__DEBUG_PRINT2("EReqGetCompositionBuffer: Getting iCompositionBuffer[%d] \n", index);

						 iCompositionBuffIdx = index;
						 *configBufferIdx = iCompositionBuffIdx;
						 iCompositionBuffer[iCompositionBuffIdx].iState = EBufferCompose;
						 CompleteRequest(iPendingReq[aReqNumber][aPendingIndex].iOwningThread,iPendingReq[aReqNumber][aPendingIndex].iTClientReq,r);
						 found = ETrue;
						 break;
						 }
					}				
			  if(!found)  	//There are no free buffers schedule request for completion
					{	
					//Case of a single composition buffer. 
					if (iDisplayInfo.iNumCompositionBuffers == 1)
						{
						iCompositionBuffIdx = 0;
						 *configBufferIdx = iCompositionBuffIdx;							 
						 CompleteRequest(iPendingReq[aReqNumber][aPendingIndex].iOwningThread, iPendingReq[aReqNumber][aPendingIndex].iTClientReq, r);
						 __DEBUG_PRINT("EReqGetCompositionBuffer  The single Composition buffer is currently being used\n");
						 break;													
						}
							
				 
					for( index=0; index< KDisplayCBMax; index++)
						 {
						 if(iCompositionBuffer[index].iState == EBufferActive)
							 {	
							 iCompositionBuffIdx = index;
							 *configBufferIdx = iCompositionBuffIdx;								 
							 __DEBUG_PRINT2("EReqGetCompositionBuffer  No composition buffer available. Next available is iCompositionBuffIdx  %d.\n",iCompositionBuffIdx );
							 break;
							 }
						 }
					}			 	 			            
          	
			  break;
			  }

         case RDisplayChannel::EReqPostUserBuffer:		//DFC thread should read client message data and update a value the client will read.	                                            
            configPack = (TInt*) aArg1;
            pack[0] = *configPack; 
            configPack++;           
            pack[1] = *configPack;
			
			r = KErrArgument;
            buffer_id = pack[0];
            if ( (buffer_id > 0) && (buffer_id <= KDisplayUBMax) )
             	{
                node = FindUserBufferNode(buffer_id);
                if(node  && (!(node->iFree) && node->iChunk ) ) 
                	{
                  	__DEBUG_PRINT2("EReqPostUserBuffer Posting buffer id: %d \n",buffer_id );
                  	r = Pdd()->PostUserBuffer(node);
                  	if(r == KErrNone)
	                  	 {
	                  	 postCount = (RDisplayChannel::TPostCount*) aArg2;
	     		    	 ++iCurrentPostCount;
	                	 *postCount= iCurrentPostCount;	                	 
	           		     break;
	                  	 }
                	}
             	}
            RequestComplete(RDisplayChannel::EReqPostUserBuffer,  r); 

            break; 	        

         case RDisplayChannel::EReqWaitForPost:			 	//DFC thread should read client message data.
			postCount = (RDisplayChannel::TPostCount*) aArg1;
			iRequestedPostCount = *postCount;
									
			//Any post operation increases iCurrentPostCount instantly but the actual post completes later on.
			if( ! Pdd()->PostPending() )
				 {
				 RequestComplete(RDisplayChannel::EReqWaitForPost,  KErrNone);
				 } 					 
         	break;

        default:
			__NK_ASSERT_ALWAYS(EFalse);  // we already validated the request number
		}
    return r;	
	}


/**
	Synchronous requests processing.
	
	@param aFunction    Request function number,
	@param apArg1       Pointer to kernel message argument 0.
	@param apArg2       Pointer to kernel message argument 1.
	@param aClient		Pointer to the client thread that issued the synchronous request.
    
    @return request processing result
*/
TInt DDisplayLdd::DoControl(TInt aFunction, TAny* aArg1, TAny* aArg2, DThread* aClient)
 {
	TInt r 			  = KErrNone;
    TBool changedRot  = ETrue;
    TBufferNode* node = 0;
	TInt buffer_id;
    TInt pack[2]      = {0,0};
    TInt handle, offset;
    TInt index        = 0;
	
	RDisplayChannel::TPostCount 		*postCount ;
	RDisplayChannel::TDisplayRotation   *rotation; 
	
	TInt	*configPack; 
	TInt	*bufferId; 	
	
	TBool   *rotationChanged;
	TInt    *idx;
	
    switch (aFunction)
		{
	    case RDisplayChannel::ECtrlPostCompositionBuffer: 	//DFC thread updates a value the client thread should read.
			postCount = (RDisplayChannel::TPostCount*) aArg2;		
					
	        node =  &iCompositionBuffer[iCompositionBuffIdx];
            r = Pdd()->PostCompositionBuffer(node);
		    if(r == KErrNone)
		    	{
    		    ++iCurrentPostCount;
    			*postCount = iCurrentPostCount;
				}
            else 
            	{
            	r = KErrGeneral;	
            	}            	    	            	
			break;

        case RDisplayChannel::ECtrlPostLegacyBuffer:		//DFC thread updates a value the client thread should read.
			postCount = (RDisplayChannel::TPostCount*) aArg2;
            r= Pdd()->PostLegacyBuffer();
            if ( r == KErrNone)
            	{
             	++iCurrentPostCount;         	
            	*postCount = iCurrentPostCount;
				}            
			break;

	    case RDisplayChannel::ECtrlRegisterUserBuffer:	//DFC thread should read client message data and update a value the client will read.
            node = FindUserBufferNode(0);
			if(node)
				{					                         
                configPack = (TInt*) aArg1;
                handle 	   = *configPack;
                configPack++;
                offset     = *configPack;                               
                r 	       = CheckAndOpenUserBuffer(node, handle, offset, aClient);
                
                if(r == KErrNone)
                	{
                	bufferId  = (TInt*) aArg2;
                	*bufferId = node->iBufferId;
                	}					
				}
            else
            	{
    			r = KErrTooBig;
            	}
			break;

	    case RDisplayChannel::ECtrlDeregisterUserBuffer:	//DFC thread should read client message data.
            bufferId  = (TInt*) aArg1;
            buffer_id = *bufferId;
            
            
			r = KErrArgument;          
            if ( (buffer_id > 0) && (buffer_id <= KDisplayUBMax) )
            	{
	            node = FindUserBufferNode(buffer_id);	            
	            if(node  && (!(node->iFree) && node->iChunk ) ) 
	            	{	                
	                if(node->iState==EBufferFree  || node->iState==EBufferCompose )
	                	{
	                	r = FreeUserBufferNode(node);	                	
	                	}
	                else
	                	{
	                	r = KErrInUse;
	                	}
	            	}               	                                    
            	}
			break;

	    case RDisplayChannel::ECtrlSetRotation:	    //DFC thread should read client message data and update a value the client will read.	    			    
            {            	         
            RDisplayChannel::TDisplayRotation rot;
            RDisplayChannel::TDisplayRotation previousRot = iCurrentRotation;
                        
            rotation  = (RDisplayChannel::TDisplayRotation*) aArg1;
           	rot 	  = *rotation;
            
            __DEBUG_PRINT3("ECtrlSetRotation previousRot= %d and rot =%d \n",previousRot, rot );
             	
            r = Pdd()->SetRotation(rot);                  
            changedRot = (previousRot != iCurrentRotation);          
            if( r == KErrNone)
    	        {                  
           	    rotationChanged  = (TBool*) aArg2;
           	    *rotationChanged = changedRot ;         	    
           	    }					    	
    	    break;
            }
            
	    case RDisplayChannel::ECtrlCurrentRotation:				//DFC thread updates a value the client thread should read.
			rotation  = (RDisplayChannel::TDisplayRotation*) aArg1;
			*rotation = iCurrentRotation; 
			break;

	    case RDisplayChannel::ECtrlGetCompositionBufferInfo:	//DFC thread should read client message data and update a value the client will read.
            idx   = ( TInt * ) aArg1;
            index = *idx;
	  
			if( (index >= (TInt) iDisplayInfo.iNumCompositionBuffers ) || (index < 0 ) )
            	{
            	r = KErrArgument;
            	break;	
            	}
			r = Kern::MakeHandleAndOpen(aClient, iCompositionBuffer[index].iChunk);

			if(r >= KErrNone)
				{
				pack[0] 	= r;
				pack[1] 	= iCompositionBuffer[index].iOffset;	
				
				configPack  = (TInt * ) aArg2;
				*configPack = pack[0];
				configPack++;
				*configPack = pack[1];				 
				
				r = KErrNone;
				}					
		 	break;

#ifdef _DEBUG
	    case RDisplayChannel::ECtrlCreateUserBuffer:
	        {
	        TUint32 chunkMapAttr;
	        TLinAddr chunkBase;
	        TPhysAddr physicalAddr;
	        RDisplayChannel::TBufferFormat bufferFormat;

	        // Read the information from the user thread pertaining to the buffer to be allocated
	        Kern::ThreadRawRead(aClient, aArg1, &bufferFormat, sizeof(bufferFormat));

	        // Allocate a chunk that can be used as a user buffer.  Don't worry about the # of bytes
	        // per pixel as this is UDEB only test code - just set it to 4 and that will ensure that
	        // it is large enough
	        TChunkCreateInfo chunkCreateInfo;
	        chunkCreateInfo.iType = TChunkCreateInfo::ESharedKernelSingle;
#ifndef __WINS__
	        chunkCreateInfo.iMapAttr = EMapAttrFullyBlocking;
#endif // ! __WINS__
	        chunkCreateInfo.iOwnsMemory = ETrue;
	        chunkCreateInfo.iDestroyedDfc = NULL;
	        chunkCreateInfo.iMaxSize = (bufferFormat.iSize.iWidth * bufferFormat.iSize.iHeight * 4);

	        if ((r = Kern::ChunkCreate(chunkCreateInfo, iChunk, chunkBase, chunkMapAttr)) == KErrNone)
	            {
	            // Commit some contiguous physical RAM for use in the chunk 
	            r = Kern::ChunkCommitContiguous(iChunk, 0, chunkCreateInfo.iMaxSize, physicalAddr);

	            // And open a handle to the chunk that will be returned to user side for use in the user
	            // side's RChunk object
	            if (r == KErrNone)
	                r = Kern::MakeHandleAndOpen(aClient, iChunk);
	            else
	            	{
	                Kern::ChunkClose(iChunk);
	                iChunk = NULL;
	            	}
	            }

	        break;
	        }
#endif // _DEBUG

	    default:
			__NK_ASSERT_ALWAYS(EFalse);  // we already validated the request number
		};
    return r;	
	}


/**
    Open a shared Chunk for the User buffer and then set the appropriate values for the
    User buffer node attributes.
	
*/
TInt DDisplayLdd::CheckAndOpenUserBuffer(TBufferNode* aNode, TInt aHandle, TInt aOffset, DThread* aClient)
 {
 
  TInt     	size 				= 0;
  DChunk*  	chunk 				= 0;
  TLinAddr 	kernelAddress 		= 0;
  TUint32  	mapAttr 			= 0;
  TUint32  	physicalAddress 	= 0;
  TUint32  	*physicalPageList 	= 0;
  TInt 		r 					= KErrBadHandle;
	
  NKern::ThreadEnterCS();
  chunk = Kern::OpenSharedChunk(aClient, aHandle, EFalse);
  NKern::ThreadLeaveCS();
  if(chunk)
    	{                            
		
		// Using iOffsetBetweenLines rather than iWidth as the controller may be using stride
		size = iDisplayInfo.iNormal.iOffsetBetweenLines * iDisplayInfo.iNormal.iHeight;
                
        r = Kern::ChunkPhysicalAddress(chunk, aOffset, size, kernelAddress, mapAttr, physicalAddress, physicalPageList);
        if( r == KErrNone )
        	{
            aNode->iChunk 			= chunk;
            aNode->iFree  			= EFalse;
            aNode->iState 			= EBufferCompose;
            aNode->iAddress 		= (TUint32)kernelAddress;
            aNode->iHandle 			= aHandle;
            aNode->iPhysicalAddress = physicalAddress;
        	}
        else
        	{ // we have an error here, close the chunk
        	r = KErrArgument;  
            Kern::ChunkClose(chunk);
        	}
    }
  return (r);
  }


/**
    Return any free buffer when trying to register a User buffer( aBufferId ==0 )
    or return the specified User buffer( used by Deregister and PostUserBuffer).
    In the second case checks about the state of the user buffer are specific to 
    each case. 
    
*/
TBufferNode* DDisplayLdd::FindUserBufferNode(TInt aBufferId)
	{
    TBufferNode* node = 0;

    if(aBufferId == 0)
		{
		for(TInt i = 0; i < KDisplayUBMax; i++)
			{
			if(iUserBuffer[i].iFree)
				{
				node = &iUserBuffer[i];
				break;
				}
			}
		}
	else
		{		 
		 node = &iUserBuffer[aBufferId-1];
		}
    return (node);
	}


/**
    Free user buffer by reseting all the appropriate fields and closing the corresponding chunk.
*/ 
TInt DDisplayLdd::FreeUserBufferNode(TBufferNode* aNode)
	{
	__DEBUG_PRINT2("FreeUserBufferNode with aNode->iAddress %08x.\n",aNode->iAddress );
	TInt r = KErrNone;
	NKern::ThreadEnterCS();
    if(aNode->iChunk != 0)
    	{
    	r= Kern::ChunkClose(aNode->iChunk);
    	}    	
    if( r== KErrNone)
    	{
    	aNode->iState 	= EBufferFree;
    	aNode->iFree 	= ETrue;
    	aNode->iAddress = 0;
    	aNode->iSize 	= 0;
    	aNode->iHandle 	= 0;
    	aNode->iChunk 	= 0;
    	}
    else
    	{
    	__DEBUG_PRINT("Failed to close chunk\n");    	
    	}
	NKern::ThreadLeaveCS();	
    
    return r;
	}	


/**    
   Calls CompleteRequest( which internally calls Kern::QueueRequestComplete )for the specified request and with the reason passed, 
   in case such an asynchronous request is pending.  Called by both the LDD and PDD.
   
*/ 
TInt DDisplayLdd::RequestComplete(TInt aRequest, TInt aReason)
	{
	TBool flag 		  = EFalse;
	
	TInt pendingIndex =   iPendingIndex[aRequest] ;	
	
	switch (aRequest)
		{		
	    case RDisplayChannel::EReqGetCompositionBuffer:	
			{
			__DEBUG_PRINT2("RequestComplete() called with a RDisplayChannel::EReqGetCompositionBuffer request and reason %d\n",aReason );
			
	        if(iPendingReq[RDisplayChannel::EReqGetCompositionBuffer][pendingIndex].iTClientReq )
				{
				if(iPendingReq[RDisplayChannel::EReqGetCompositionBuffer][pendingIndex].iTClientReq->IsReady() )
					{
		            flag = ETrue;					
					}
	        	}
	        break;											
			}

		 case RDisplayChannel::EReqWaitForPost:		        
			{
			__DEBUG_PRINT2("RequestComplete() called with a RDisplayChannel::EReqWaitForPost request and reason %d\n",aReason );
			
	 		if(iPendingReq[RDisplayChannel::EReqWaitForPost][pendingIndex].iTClientReq) 
	    	    {
	    	    if( iPendingReq[RDisplayChannel::EReqWaitForPost][pendingIndex].iTClientReq->IsReady()  && (iCurrentPostCount >= iRequestedPostCount) )
	    	    	{
					flag = ETrue;	    	    	
	    	    	}
	            }
	        break;    												
			}
 		
		case RDisplayChannel::EReqPostUserBuffer:	
			{
			__DEBUG_PRINT2("RequestComplete() called with a RDisplayChannel::EReqPostUserBuffer request and reason %d\n",aReason );
				
			if(iPendingReq[RDisplayChannel::EReqPostUserBuffer][pendingIndex].iTClientReq)
	    		{
	    		if( iPendingReq[RDisplayChannel::EReqPostUserBuffer][pendingIndex].iTClientReq->IsReady() )
	    			{
					flag = ETrue;	    			
	    			}
	            }
	        break;			
			}
		default:
			__DEBUG_PRINT("RequestComplete() called for an unknown request\n");
			return KErrGeneral;
		
		}
		
	if (flag)
		{
		CompleteRequest(iPendingReq[aRequest][pendingIndex].iOwningThread,iPendingReq[aRequest][pendingIndex].iTClientReq,aReason);		
		}
				
	return KErrNone;	
	}


/** 
Complete an asynchronous request back to the client.

@param aThread     The client thread which issued the request.
@param aTClientReq Pointer reference to the TClientRequest object  
@param aReason     The request status code.  

@pre The thread must be in a critical section. 
*/
void DDisplayLdd::CompleteRequest(DThread* aThread, TClientRequest*& aTClientReq, TInt aReason)
	{			
	__DEBUG_PRINT4("Complete aTClientReq %08x with reason %d for aThread = %08x\n", aTClientReq, aReason,aThread );		
	
	Kern::QueueRequestComplete(aThread,aTClientReq,aReason);
		
	aThread->AsyncClose();	// Asynchronously close our reference on the client thread - don't want to be blocked if this is final reference. 
				
	aThread		  =0;
	aTClientReq   =0;
	
#ifdef _DEBUG	
	__e32_atomic_add_ord32(&iThreadOpenCount, TUint32(-1));
	__e32_atomic_add_ord32(&iAsyncReqCount,   TUint32(-1));
#endif	
			
	}



/**
	static factory function for the LDD.
	
	@return pointer to the created (or existing) instance of the class
*/
DDisplayLdd* DDisplayLdd::CreateInstance()
	{
	__DEBUG_PRINT("DDisplayLdd::CreateInstance()\n");
	 // create LDD channel instance
    DDisplayLdd* obj = new DDisplayLdd();
    return obj;
	
	}


/************************************************************************************
 *            LDD factory, DDisplayLddFactory class implementation
 ************************************************************************************/
 
/**
	Constructor
*/  
DDisplayLddFactory::DDisplayLddFactory()
	{
	__DEBUG_PRINT("DDisplayLddFactory::DDisplayLddFactory() \n");
	
	iParseMask 	= KDeviceAllowPhysicalDevice | KDeviceAllowUnit ;
	
    iVersion	= TVersion( KDisplayChMajorVersionNumber,
                      	    KDisplayChMinorVersionNumber,
                      	    KDisplayChBuildVersionNumber);
                      	    
	iUnitsOpenMask =0;                      	    
	}


/**
    Destructor
*/
DDisplayLddFactory::~DDisplayLddFactory()
	{
	}


/**
	static factory function for the LDD factory.
	
	@return pointer to the created instance of the class
*/
DDisplayLddFactory* DDisplayLddFactory::CreateInstance()

	{
	__DEBUG_PRINT("DDisplayLddFactory::CreateInstance() \n");
	 
	 DDisplayLddFactory* obj = new DDisplayLddFactory;
    return obj;
	}


/**
    Set our name and return error code
*/
TInt DDisplayLddFactory::Install()

	{
	__DEBUG_PRINT("DDisplayLddFactory::Install() \n");
    return SetName(&RDisplayChannel::Name());
	}


void DDisplayLddFactory::GetCaps(TDes8& /*aDes*/) const

	{
	//No action.
	}


/**
	LDD factory function. Creates LDD object.
	
	@param  aChannel  A pointer to an LDD channel object which will be initialised on return.
	
	@return KErrNone  if object successfully allocated, KErrNoMemory if not.			
*/
TInt DDisplayLddFactory::Create(DLogicalChannelBase*& aChannel)
	{
	__DEBUG_PRINT("DDisplayLddFactory::Create \n");
    aChannel = DDisplayLdd::CreateInstance();
    return (!aChannel)? KErrNoMemory : KErrNone;
	}



/**
Check whether a channel is currently open on the specified unit.
@param aUnit The number of the unit to be checked.
@return ETrue if a channel is open on the specified channel, EFalse otherwise.
@pre The unit info. mutex must be held.
*/
TBool DDisplayLddFactory::IsUnitOpen(TInt aUnit)
	{
	return(iUnitsOpenMask&(1<<aUnit));
	}


/**
Attempt to change the state of the channel open status for a particular channel.
@param aUnit The number of the unit to be updated.
@param aIsOpenSetting The required new state for the channel open status: either ETrue to set the status to open or 
	EFalse to set the status to closed.
@return KErrNone if the status was updated successfully, KErrInUse if an attempt has been made to set the channnel status
	to open while it is already open.
*/		
TInt DDisplayLddFactory::SetUnitOpen(TInt aUnit,TBool aIsOpenSetting)
	{
		
	NKern::FMWait(&iUnitInfoMutex); // Acquire the unit info. mutex.
		
	// Fail a request to open an channel that is already open
	if (aIsOpenSetting && IsUnitOpen(aUnit))
		{
		NKern::FMSignal(&iUnitInfoMutex); // Release the unit info. mutex.
		return(KErrInUse);
		}
	
	// Update the open status as requested
	if (aIsOpenSetting)
		iUnitsOpenMask|=(1<<aUnit);
	else
		iUnitsOpenMask&=~(1<<aUnit);
	
	NKern::FMSignal(&iUnitInfoMutex); // Release the unit info. mutex.	
	return(KErrNone);
	}


/**
	"Standard LDD" entrypoint.
	
	Is called on CreateLogicalDevice() if the user calls LoadLogicalDevice(). Creates LDD factory.
	
	@return pointer to the LDD factory object.
*/
DECLARE_STANDARD_LDD()
	{
	__DEBUG_PRINT("DECLARE_STANDARD_LDD() \n");
     DDisplayLddFactory* pLDDFactory = DDisplayLddFactory::CreateInstance();
    return  pLDDFactory;
	}




