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
// os\kernelhwsrv\kernel\eka\include\drivers\display.h 
// Interface to LDD of the Display GCE driver
// Kernel side definitions for the GCE driver
//

/**
 @file
 @internalTechnology
 @prototype
*/


#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <videodriver.h>
#include <dispchannel.h>
#include <platform.h>
#include <pixelformats.h>



const TInt KDisplayLBMax = 2;
const TInt KDisplayCBMax = 2;
const TInt KDisplayUBMax = 8;


const TInt KPendingReqArraySize  = RDisplayChannel::EReqWaitForPost +1;
  
const TInt KMaxQueuedRequests 	 = 3;

class DDisplayChannel;


enum TBufferType
{
    	EBufferTypeLegacy = 0,
    	EBufferTypeComposition,
    	EBufferTypeUser,
};


enum TBufferState
{
   		EBufferFree = 0,
   		EBufferCompose,
   		EBufferPending,
   		EBufferActive
};


typedef struct
{
    	TInt    		iType;
    	TInt    		iBufferId;
	    TBool   		iFree;
	    TInt    		iHandle;
	    TInt    		iSize;
	    TUint32 		iAddress;
	    TUint32  		iPhysicalAddress; 
	    TInt    		iOffset ;
	    DChunk * 		iChunk ;
	    TBufferState 	iState;
	    TRequestStatus* iPendingRequest;

} TBufferNode;


/**
An object encapsulating a request from the client(iOwningThread) to the GCE driver. 
*/ 
typedef struct
{
	  
	  /** The TClientRequest object associated with the request - used to signal completion of the request and pass back a
	   completion code. */
	  TClientRequest*   iTClientReq;
	  
	  /** The thread which issued the request and which supplied the request status. */
	  DThread* 			iOwningThread;
	  
} TRequestNode;



class DDisplayPdd; 


/**
  Logical Channel factory class for 'Display Channel LDD'
*/

class DDisplayLddFactory : public DLogicalDevice
	{
public:
    static DDisplayLddFactory* CreateInstance();
	~DDisplayLddFactory();
	//	Inherited from DLogicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	TBool 	IsUnitOpen(TInt aUnit);
	TInt 	SetUnitOpen(TInt aUnit,TBool aIsOpenSetting);
private:
	DDisplayLddFactory();
	
	
private:
	/** Mask to keep track of which units have a channel open on them. */
	TUint iUnitsOpenMask;				
	/** A mutex to protect access to the unit info mask. */
	NFastMutex iUnitInfoMutex;	
	};


/**
  Logical Channel class for 'Display Channel LDD'
*/
class DDisplayLdd : public DLogicalChannel 
	{

public:
    // create one instance of this object
	static DDisplayLdd* CreateInstance();
	virtual ~DDisplayLdd();
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
    virtual void HandleMsg(TMessageBase* aMsg);

private:
	DDisplayLdd();

private:
	// Implementation for the differnt kinds of messages sent through RBusLogicalChannel
	TInt  			DoControl(TInt aFunction, TAny* a1, TAny* a2, DThread* aClient);
	TInt  			DoRequest(TInt aReqNo, TAny* a1, TAny* a2,  TInt index, DThread* aClient);
	void  			DoCancel(TUint aMask);
	
	TInt 		    SendRequest(TMessageBase* aMsg);
	TInt 		    SendControl(TMessageBase* aMsg);
	TInt 		    SendMsg(TMessageBase* aMsg);	 
       
    TBufferNode*  	FindUserBufferNode(TInt aBufferId);  
    TInt 			CheckAndOpenUserBuffer(TBufferNode* aNode, TInt aHandle, TInt aOffset, DThread* aClient);         
    TInt  			FreeUserBufferNode(TBufferNode* aNode);
    
    void 			CompleteRequest(DThread* aThread, TClientRequest*& aTClientReq, TInt aReason);
          
    DDisplayPdd * 	Pdd();  
    	
public: 	
 	virtual TInt 	RequestComplete(TInt aRequest, TInt );


public:
              	    
    // display info
    RDisplayChannel::TDisplayInfo 	iLegacyInfo;
    RDisplayChannel::TDisplayInfo 	iDisplayInfo;
    
        // post counters
    RDisplayChannel::TPostCount   	iCurrentPostCount;
    RDisplayChannel::TPostCount   	iRequestedPostCount;
    
    
    DThread*    	iClient;	
	TInt        	iUnit;      
    
    // frame buffer nodes
    TBufferNode 	iLegacyBuffer[KDisplayLBMax];
    TBufferNode 	iCompositionBuffer[KDisplayCBMax];
    TBufferNode 	iUserBuffer[KDisplayUBMax];
     
    //pending queue for asynchronous requests
    TRequestNode 	iPendingReq[KPendingReqArraySize][KMaxQueuedRequests];
        
    //Queue of TClientRequest objects, one for each type of asynchronous request.
    TClientRequest* iClientRequest[KPendingReqArraySize][KMaxQueuedRequests];
	
	//The index in structures iPendingReq and iClientRequest that identifies the active TClientRequest object.
	//For each type of asynchronous request, iPendingIndex is the index of the active TClientRequest object 
	//in iPendingReq
	TInt			iPendingIndex[KPendingReqArraySize];
    
    // Protect access of iClientRequest
    DMutex * 		 iClientRequestMutex;
    
     
    // current index
    TInt    		iLegacyBuffIdx;
	TInt    		iCompositionBuffIdx;
    TInt    		iUserBuffIdx;
     
    
    RDisplayChannel::TDisplayRotation 	iLegacyRotation;
    RDisplayChannel::TDisplayRotation   iCurrentRotation;
    
    
    TBool    		iReady;  
    
    /** Used in debug builds to track that all calls to DThread::Open() are balanced with a close before the driver closes. */
	TInt iThreadOpenCount; 
	
	/** Used in debug builds to track the number of asynchronous requests that are queued is equal to the number of 
	requests that are completed, before the driver closes.*/
	TInt iAsyncReqCount; 

	/** Chunk used in UDEB only for testing user buffers. */
	DChunk*	iChunk;
	};


 /**
    Display PDD base class with GCE support.
   */

class DDisplayPdd : public DBase
	{

	public:
    
  	/**  
    Called by the LDD to handle the device specific part of switching to Legacy mode.
 
	@return KErrNone if successful; or one of the other system wide error codes.
    */    
    virtual TInt  SetLegacyMode()=0;
        
     /**
     Called by the LDD to handle the device specific part of switching to GCE mode.
     
     @return KErrNone if successful; or one of the other system wide error codes.
     */       
    virtual TInt  SetGceMode()=0;
    
     /**
     Called by the LDD to handle the device specific part of setting the rotation.
     
     @return KErrNone if successful; or one of the other system wide error codes.
     */       
    virtual TInt  SetRotation(RDisplayChannel::TDisplayRotation aRotation)=0;

     /**
     Called by the LDD to handle the device specific part of posting a User Buffer.
     
     @return KErrNone if successful; or one of the other system wide error codes.
     */ 	
	virtual TInt  PostUserBuffer(TBufferNode* aNode)=0;
	
     /**
     Called by the LDD to handle the device specific part of posting a Composition Buffer
     
     @return KErrNone if successful; or one of the other system wide error codes.
     */ 		
	virtual TInt  PostCompositionBuffer(TBufferNode* aNode)=0;
        
     /**
     Called by the LDD to handle the device specific part of posting the Legacy Buffuer
     
     @return KErrNone if successful; or one of the other system wide error codes.
     */   
    virtual TInt  PostLegacyBuffer()=0;
    
    /**
     Called by the LDD to handle device specific cleanup operations when a channel is closed.
          
     @return KErrNone if successful; or one of the other system wide error codes.
     */  
    virtual TInt  CloseMsg()=0;
            
     /**
     Called by the LDD to handle device specific initialisation tasks when a channel is opened.
     
     @param aUnit The screen/hardware unit number.
     @return KErrNone if successful; or one of the other system wide error codes.
     */    
    virtual TInt  CreateChannelSetup(TInt aUnit)=0;
          
     /**
     Called by the LDD in order to detect whether a post operation is pending. This type of 
     information is specific to the actual physical device.
     
     @return ETrue if a Post operation is pending otherwise EFalse.
     */       
   	virtual TBool  PostPending()=0;
    
    /**
     Called by the LDD to retrieve the DFC Queue created in the PDD.      
     
     @param aUnit The screen/hardware unit number.
     @return A pointer to the TDfcQue object created in the PDD.
     */    
    virtual TDfcQue* DfcQ(TInt  aUnit)=0;    
            
     /**
     Called by the PDD when an asynchronous request should be completed with a specific reason.
     (Just calls the LDD's RequestComplete method)
     
      @param aRequest Any value from the RDisplayChannel::TRequest enumeration.
      @param aReason  Any valid error reason.
      
      @return KErrNone if successful; or one of the other system wide error codes.
     */
    inline TInt RequestComplete(TInt aRequest, TInt aReason );
    

public:        
  	/**
    A pointer to the logical device driver's channel that owns this device.
    */        
    DDisplayLdd *iLdd;
    /**
	Every post operation sets this flag to true in order to identify when
	the previsouly posted buffer is no longer in use by the display hardware. 
    */ 
	TBool		 iPostFlag;
           
	};


inline DDisplayPdd * DDisplayLdd::Pdd()
    { return (DDisplayPdd*) iPdd; }


inline TInt DDisplayPdd::RequestComplete(TInt aRequest, TInt aReason)
	{ return iLdd->RequestComplete(aRequest,aReason); }




//#define _GCE_DISPLAY_DEBUG

#ifdef _GCE_DISPLAY_DEBUG

#define  __DEBUG_PRINT(a) 			Kern::Printf(a)
#define  __DEBUG_PRINT2(a,b) 		Kern::Printf(a,b)
#define  __DEBUG_PRINT3(a,b,c) 		Kern::Printf(a,b,c)
#define  __DEBUG_PRINT4(a,b,c,d) 	Kern::Printf(a,b,c,d)
#define  __DEBUG_PRINT5(a,b,c,d,e) 	Kern::Printf(a,b,c,d,e)

#else
#define  __DEBUG_PRINT(a)
#define  __DEBUG_PRINT2(a,b)
#define  __DEBUG_PRINT3(a,b,c)
#define  __DEBUG_PRINT4(a,b,c,d) 
#define  __DEBUG_PRINT5(a,b,c,d,e)

#endif


#endif	// __DISPLAY_H__ 
