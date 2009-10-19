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
// e32\drivers\display\display.cpp  
// LDD for a Display driver with GCE support
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



static const char KDisplayLddPanic[]="DISPLAY/GCE LDD";


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
	}


DDisplayLdd::~DDisplayLdd()
	{
	__DEBUG_PRINT("DDisplayLdd::~DDisplayLdd()  \n"); 
     // cancel outstanding requests
    for(TInt k = 0; k < KPendingReqArraySize ; k++)
    	{
        if(iPendingReq[k].iStatus)
        	{
     		CompleteRequest(iPendingReq[k].iOwningThread,iPendingReq[k].iStatus,KErrCancel);        	
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
    
    __ASSERT_DEBUG(iThreadOpenCount==0,Kern::Fault(KDisplayLddPanic,__LINE__)); 
    	
    // Clear the 'units open mask' in the LDD factory.
	if (iUnit>=0)
		((DDisplayLddFactory*)iDevice)->SetUnitOpen(iUnit,EFalse);	 	
	}
 

/**
	LDD second stage constructor
*/
TInt DDisplayLdd::DoCreate(TInt aUnit, const TDesC8* /* anInfo*/, const TVersion& aVer)
	{ 
	
	__DEBUG_PRINT("DDisplayLdd::DoCreate()\n"); 

	if(    !Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,      __PLATSEC_DIAGNOSTIC_STRING("Checked by DISPLAY.LDD (GCE Driver)") )
		&& !Kern::CurrentThreadHasCapability(ECapabilityReadDeviceData, __PLATSEC_DIAGNOSTIC_STRING("Checked by DISPLAY.LDD (GCE Driver)") )
		&& !Kern::CurrentThreadHasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by DISPLAY.LDD (GCE Driver)") )
		&& !Kern::CurrentThreadHasCapability(ECapabilityProtServ,       __PLATSEC_DIAGNOSTIC_STRING("Checked by DISPLAY.LDD (GCE Driver)") ) )
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
		{
		return r;
		}
				   	       
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

    //Initialise pending queue for asynchronous requests
    for(int k = 0; k < KPendingReqArraySize; k++) 
    	{
      	iPendingReq[k].iStatus = 0;
      	iPendingReq[k].iOwningThread = 0;
    	}
          
    Pdd()->SetGceMode();	
	SetDfcQ(Pdd()->DfcQ(aUnit));	   
    iMsgQ.Receive();
    
    return KErrNone;		
}


/**
 * All driver's client requests (synchronous and asynchronous) are sent as
 * kernel messages by generic kernel to logical channel. This function
 * catches messages sent by the generic kernel
 *
 * @param aMsg KErnel side thread messaging
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
        for(int k = 0; k < KPendingReqArraySize; k++)
        	{
            if(iPendingReq[k].iStatus) 
            	{
            	CompleteRequest(iPendingReq[k].iOwningThread,iPendingReq[k].iStatus,KErrCancel);
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
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		TInt r = DoRequest(~id,pS,m.Ptr1(),m.Ptr2(), client);
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
	        if(iPendingReq[aReqNumber].iStatus)
	        	{
        		CompleteRequest(iPendingReq[aReqNumber].iOwningThread,iPendingReq[aReqNumber].iStatus,KErrCancel);
        		
        		iPendingReq[aReqNumber].iStatus 	  = 0;
        		iPendingReq[aReqNumber].iOwningThread = 0;
            	}
			break;
		}	
	
	}

/**
	Asynchronous request processing.
	
	@param aFunction    request function number
    @param apRqStat     pointer to the user's request status object.
    @param apArg1       pointer to the 1st parameter
    @param apArg2       pointer to the 2nd parameter
	
	@return request scheduling result, system-wide error code.
*/
TInt DDisplayLdd::DoRequest(TInt aReqNumber, TRequestStatus* aRqStat, TAny* aArg1, TAny* aArg2, DThread* aClient)
	{	
    TUint count;
    TInt pack[2];
    TInt r 				= KErrNone;
    TBufferNode* node 	= 0;
    TInt buffer_id  	= 0;
        
    // Open a reference on the client thread while the request is pending so it's control block can't disappear until this driver has finished with it.
	r=aClient->Open();
	__ASSERT_ALWAYS(r==KErrNone,Kern::Fault(KDisplayLddPanic,__LINE__));
#ifdef _DEBUG
	__e32_atomic_add_ord32(&iThreadOpenCount, 1);
#endif	
    
    // check if this request is valid
    if(aReqNumber >= 0 && aReqNumber <= RDisplayChannel::EReqWaitForPost)
    	{
        // cancel outstanding request
        if(iPendingReq[aReqNumber].iStatus)
			{
            CompleteRequest(iPendingReq[aReqNumber].iOwningThread,iPendingReq[aReqNumber].iStatus,KErrCancel);
            iPendingReq[aReqNumber].iStatus 	  = 0;
            iPendingReq[aReqNumber].iOwningThread = 0;
        	}
        	
    	// store request and client
    	iPendingReq[aReqNumber].iStatus = aRqStat;
    	iPendingReq[aReqNumber].iOwningThread = aClient;
    	}

    switch (aReqNumber)
		{
	     case RDisplayChannel::EReqGetCompositionBuffer:

			if(aArg1 == 0 )
	              {
	               r = KErrGeneral;
	               CompleteRequest(iPendingReq[aReqNumber].iOwningThread,iPendingReq[aReqNumber].iStatus,r);
	               iPendingReq[aReqNumber].iStatus 		 = 0;
	               iPendingReq[aReqNumber].iOwningThread = 0;
	              }			
			else
            	  {	 	
	              TInt  index;
	              TBool found = EFalse;
	              
	              for(index =0; index< KDisplayCBMax; index++)
		              	{
		              	if(iCompositionBuffer[index].iState == EBufferFree || iCompositionBuffer[index].iState == EBufferCompose )
							{
							__DEBUG_PRINT2("EReqGetCompositionBuffer: Getting iCompositionBuffer[%d] \n", index);

			                 iCompositionBuffIdx = index;
			                 r = Kern::ThreadRawWrite(aClient, aArg1, &iCompositionBuffIdx, sizeof(TInt), aClient);
							 if ( r == KErrNone)
								{   
								 iCompositionBuffer[iCompositionBuffIdx].iState = EBufferCompose;
								 CompleteRequest(iPendingReq[aReqNumber].iOwningThread,iPendingReq[aReqNumber].iStatus,r);
								 iPendingReq[aReqNumber].iStatus 	   = 0;
								 iPendingReq[aReqNumber].iOwningThread = 0;
								 found = ETrue;
								 break;
								}
							 }
		              	}				
				  if(!found)  	//There are no free buffers schedule request for completion
					 	{	
						//Case of a single composition buffer. 
						if (iDisplayInfo.iNumCompositionBuffers == 1)
							{
							iCompositionBuffIdx = 0;
			             	 r = Kern::ThreadRawWrite(aClient, aArg1, &iCompositionBuffIdx, sizeof(TInt), aClient);
		                     __DEBUG_PRINT("EReqGetCompositionBuffer  The single Composition buffer is currently being used\n");
		                     break;													
							}
								
				 	 
		              	for( index=0; index< KDisplayCBMax; index++)
			                 {
			                 if(iCompositionBuffer[index].iState == EBufferActive)
								 {	
		                         iCompositionBuffIdx = index;
			                  	 r = Kern::ThreadRawWrite(aClient, aArg1,&iCompositionBuffIdx, sizeof(TInt), aClient);
			                     __DEBUG_PRINT2("EReqGetCompositionBuffer  No composition buffer available. Next available is iCompositionBuffIdx  %d.\n",iCompositionBuffIdx );
			                     break;
								 }
			                 }
				 	 	}			 	 		
	            }
          	break;

         case RDisplayChannel::EReqPostUserBuffer:			
            r= Kern::ThreadRawRead(aClient, aArg1, &pack, (sizeof(TInt)*2));
			if ( r == KErrNone)
				{
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
		     		    	 ++iCurrentPostCount;
		                	 r=Kern::ThreadRawWrite(aClient, aArg2, (const TAny *)&iCurrentPostCount, sizeof(RDisplayChannel::TPostCount), aClient);	           		     
		           		     break;
		                  	 }
	                	}
	             	}
	            RequestComplete(RDisplayChannel::EReqPostUserBuffer,  r); 
				}
            break; 	        

         case RDisplayChannel::EReqWaitForPost:
            r= Kern::ThreadRawRead(aClient, aArg1,  &count, sizeof(RDisplayChannel::TPostCount));
			if ( r == KErrNone)
				{   
			    iRequestedPostCount = count;
	    	    //Any post operation increases iCurrentPostCount instantly but the actual post completes later on.
	    	    if( ! Pdd()->PostPending() )
	    	    	 {
	    	    	 RequestComplete(RDisplayChannel::EReqWaitForPost,  KErrNone);
	    	    	 } 
				}	 
         	break;

        default:
			r = KErrNotSupported;
			break;
		}
    return r;	
	}


/**
	Synchronous requests processing.
	
	@param aFunction    request function number,
	@param apArg1       pointer to the 1st parameter
	@param apArg2       pointer to the 2n parameter
    
    @return request processing result
*/
TInt DDisplayLdd::DoControl(TInt aFunction, TAny* aArg1, TAny* aArg2, DThread* aClient)
 {
	TInt r 			  = KErrNotSupported;
    TBool changedRot  = ETrue;
    TBufferNode* node = 0;
	TInt buffer_id;
    TInt pack[2]      = {0,0};
    TInt handle, offset;
    TInt index        = 0;

    TPckgBuf<RDisplayChannel::TDisplayInfo> pckgInfo(iDisplayInfo);

    switch (aFunction)
		{
	    case RDisplayChannel::ECtrlGetDisplayInfo:
            r=Kern::ThreadRawWrite(aClient, aArg1, (const TAny *)&pckgInfo, sizeof(pckgInfo), aClient);
    	    break;

	    case RDisplayChannel::ECtrlPostCompositionBuffer:
	        node =  &iCompositionBuffer[iCompositionBuffIdx];
            r = Pdd()->PostCompositionBuffer(node);
		    if(r == KErrNone)
		    	{
    		    ++iCurrentPostCount;
                r=Kern::ThreadRawWrite(aClient, aArg2, (const TAny *)&iCurrentPostCount, sizeof(RDisplayChannel::TPostCount), aClient);
    			}
            else 
            	{
            	r = KErrGeneral;	
            	}            	    	            	
			break;

        case RDisplayChannel::ECtrlPostLegacyBuffer:
            r= Pdd()->PostLegacyBuffer();
            if ( r == KErrNone)
            	{
             	++iCurrentPostCount;
            	r = Kern::ThreadRawWrite(aClient, aArg2, (const TAny *)&iCurrentPostCount, sizeof(RDisplayChannel::TPostCount), aClient);           	
            	}            
			break;

	    case RDisplayChannel::ECtrlRegisterUserBuffer:
            node = FindUserBufferNode(0);
			if(node)
				{
                r= Kern::ThreadRawRead(aClient, aArg1, &pack, (sizeof(TInt)*2));
				if(r == KErrNone)
					{
	                handle  = pack[0]; 
	                offset  = pack[1];
	                r 		= CheckAndOpenUserBuffer(node, handle, offset, aClient);
	                
	                if(r == KErrNone)
	                	{
	                     r= Kern::ThreadRawWrite(aClient, aArg2, (const TAny *)&node->iBufferId, sizeof(TInt), aClient);
	                	}
					}
				}
            else
            	{
    			r = KErrTooBig;
            	}
			break;

	    case RDisplayChannel::ECtrlDeregisterUserBuffer:
            r= Kern::ThreadRawRead(aClient, aArg1, &buffer_id, sizeof(TInt));
            if ( r == KErrNone)
				{
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
				}
			break;

	    case RDisplayChannel::ECtrlPostCount:
            r= Kern::ThreadRawWrite(aClient, aArg1, (const TAny *)&iCurrentPostCount, sizeof(TUint), aClient);
			break;

	    case RDisplayChannel::ECtrlSetRotation:	    
	    	{	    			    
            RDisplayChannel::TDisplayRotation rot;
            TInt previousRot = iCurrentRotation;
            r= Kern::ThreadRawRead(aClient, aArg1, &rot, sizeof(RDisplayChannel::TDisplayRotation));
            if ( r == KErrNone)
				{               	
                r = Pdd()->SetRotation((TInt)rot);                  
                changedRot=(previousRot!=iCurrentRotation);              
                if( r == KErrNone)
        	        {
                   	 r= Kern::ThreadRawWrite(aClient, aArg2, (const TAny *)&changedRot, sizeof(TBool), aClient);                       
               	    }
				}
	    	}
    	    break;

	    case RDisplayChannel::ECtrlCurrentRotation:
			r=Kern::ThreadRawWrite(aClient, aArg1, (const TAny *)&iCurrentRotation, sizeof(RDisplayChannel::TDisplayRotation), aClient);
			break;

	    case RDisplayChannel::ECtrlGetCompositionBufferInfo:
    	    {
            r= Kern::ThreadRawRead(aClient, aArg1, &index, sizeof(TInt));                     
            if ( r == KErrNone)
				{   
				if( (index >= KDisplayCBMax ) || (index < 0 ) )
	            	{
	            	r = KErrArgument;
	            	break;	
	            	}
				r = Kern::MakeHandleAndOpen(aClient, iCompositionBuffer[index].iChunk);

				if(r >= KErrNone)
					{
					pack[0] = r;
					pack[1] = iCompositionBuffer[index].iOffset;
					r=Kern::ThreadRawWrite(aClient, aArg2, &pack, (sizeof(TInt)*2), aClient);
					}
				}	
		 	break;
			}
    	   
        default:
			r = KErrNotSupported;
			break;
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
   Calls CompleteRequest( which internally sends a Kern::RequestComplete message )for the specified request and with the reason passed, 
   in case such an asynchronous request is pending. Also resets the pending queue fields for that request if it was actually pending. 
   Called by both the LDD and PDD.
   
*/ 
TInt DDisplayLdd::RequestComplete(TInt aRequest, TInt aReason)
	{
	TBool flag = EFalse;
			
	switch (aRequest)
		{
	    case RDisplayChannel::EReqGetCompositionBuffer:	
			{
			__DEBUG_PRINT2("RequestComplete() called with a RDisplayChannel::EReqGetCompositionBuffer request and reason %d\n",aReason );
			
	        if(iPendingReq[RDisplayChannel::EReqGetCompositionBuffer].iStatus)
				{
				__DEBUG_PRINT("RequestComplete(): Calling CompleteRequest EReqGetCompositionBuffer \n");
	            flag = ETrue;
	        	}
	        break;											
			}

		 case RDisplayChannel::EReqWaitForPost:		        
			{
			__DEBUG_PRINT2("RequestComplete() called with a RDisplayChannel::EReqWaitForPost request and reason %d\n",aReason );
			
	 		if((iPendingReq[RDisplayChannel::EReqWaitForPost].iStatus != 0) && (iCurrentPostCount >= iRequestedPostCount) )
	    	    {
	    	    __DEBUG_PRINT("RequestComplete(): Calling CompleteRequest EReqWaitForPost \n");
				flag = ETrue;
	            }
	        break;    												
			}
 		
		case RDisplayChannel::EReqPostUserBuffer:	
			{
			__DEBUG_PRINT2("RequestComplete() called with a RDisplayChannel::EReqPostUserBuffer request and reason %d\n",aReason );
				
			if((iPendingReq[RDisplayChannel::EReqPostUserBuffer].iStatus != 0) )
	    		{
	    	    __DEBUG_PRINT("RequestComplete(): Calling CompleteRequest EReqPostUserBuffer \n");
				flag = ETrue;
	            }
	        break;			
			}
		default:
			__DEBUG_PRINT("RequestComplete() called for an unknown request\n");
			return KErrGeneral;
		
		}
		
	if (flag)
		{
		CompleteRequest(iPendingReq[aRequest].iOwningThread,iPendingReq[aRequest].iStatus,aReason);
		iPendingReq[aRequest].iStatus 		= 0;
		iPendingReq[aRequest].iOwningThread = 0;			
		}
				
	return KErrNone;	
	}


/** 
Complete an asynchronous request back to the client.
@param aThread The client thread which issued the request.
@param aStatus The TRequestStatus instance that will receive the request status code. 
@param aReason The request status code.  
@pre The thread must be in a critical section. 
*/
void DDisplayLdd::CompleteRequest(DThread* aThread, TRequestStatus*& aStatus, TInt aReason)
	{		
	Kern::RequestComplete(aThread,aStatus,aReason);		// Complete the request back to the client.
	
	aThread->AsyncClose();	// Asynchronously close our reference on the client thread - don't want to be blocked if this is final reference. 

#ifdef _DEBUG	
	__e32_atomic_add_ord32(&iThreadOpenCount, TUint32(-1));
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




