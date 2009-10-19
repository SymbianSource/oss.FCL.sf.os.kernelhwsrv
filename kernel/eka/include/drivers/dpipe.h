// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//




#ifndef __DPIPE_H__
#define __DPIPE_H__

#define _TEST__

#if !defined(__KERNEL_H__)
#include <kernel/kernel.h>
#endif

#include <rpipe.h>

const TInt  KIdBase		= 0x0;

class DPipe;

class DPipeDevice : public DLogicalDevice
/**
The factory class is derived from Dlogical device. The user side calls 
User::LoadLogicalDevice() to load the LDD dll and create the LDD 
factory object in the kernel heap.
@internalTechnology
*/
{
public:
	/**
	 Set the version number
	 */
    DPipeDevice();

    ~DPipeDevice();

    // Inherited from DLogicalDevice
	/**
	 Second stage constructor and at least set a name for the 
	 driver object.
	 */
    virtual TInt Install();


    virtual void GetCaps(TDes8& aDes) const;

	/**
	 Called by the Kernel's Device driver framework to create a logical
	 Channel. This called in the context of the user thread. which requested
	 the creation of the logical channel.It checks if maximum pipe creation 
	 has reached before creating a new Kernel pipe object.
	 @param aChannel Set to point to the created logical channel

	 @return KErrNone if successful, otherwise system wide error codes.
	 */
    virtual TInt Create(DLogicalChannelBase*& aChannel);

	
	/**
	 Called by the Logical channel instance to create DPipe  and
	 associate itself.
	 */
	TInt  CreatePipe(const TDesC& aName, TInt aSize, DPipe*& aPipe, TAny* aCapCheck = NULL);

	DPipe* CreatePipe(TInt aSize);
	
	DPipe* FindNamedPipe(const TDesC* aName);

	DPipe* FindUnnamedPipe(const TInt aId);

	TInt Destroy(const TDesC* aName);

	TInt Close(TInt aId);
	
	
	inline DMutex& Mutex()
		{
		return *iMutex;
		}

	inline void Wait()
		{
		Kern::MutexWait(*iMutex);
		}
	
	inline void Signal()
		{
		Kern::MutexSignal(*iMutex);
		}
  
 private:
 	TInt GenerateId();
	
	TInt AddPipe(DPipe* aObj);
	
    void RemovePipe(DPipe** aObj);
    
    
private:
	 //! Represents the Data in a pipe.
	DPipe **iDpipes;
	
	DMutex *iMutex;	
	
	TInt iAllocated;
	
	TInt iCount;
	
    TInt iIdindex;

};



class DPipeChannel : public DLogicalChannelBase
/**
DPipe Channel provides the Kernel interface to the DPipe. The request from the RPipe handler 
in the context of user thread, is transfered to DPipeChannel class. This is the interface 
between the DPipe kernel object and the user request through RPipe handler. 

@internalTechnology
*/
	{
public:
     DPipeChannel();
     virtual ~DPipeChannel();

    // inherited from DObject 
    virtual TInt RequestUserHandle (DThread* aThread, TOwnerType aType);

    // inherited from DLogicalChannelBase
    virtual TInt DoCreate (TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);

	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
	
	
private:

    // The user request is mapped 
    TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
    
    TInt DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1,    
    TAny* a2);

    // This function will be called under DoControl()
    TInt PipeCreate(TAny* a1,  TAny* a2);
	
    TInt PipeCreate(TInt aSize);
	
    TInt PipeOpen(const TDesC* aName, RPipe::TChannelType aType);
    
    TInt PipeOpen (const TInt aId);
    
    TInt OpenOnReader(const TDesC* aName);
    
    TInt PipeDestroy(const TDesC* aName);

    TInt Read (TAny* aBuff, TInt aSize);

    TInt Write (TAny* aBuff, TInt aSize);
    
    TInt Size();

    TInt CloseHandle();
    
    TBool CheckCap();
    
    // Registration of the Asynchronous request
    TInt NotifySpaceAvailable (TInt aSize, TRequestStatus* aStat, TBool aAllowDisconnected);

    TInt NotifyDataAvailable (TRequestStatus* aStat, TBool aAllowDisconnected);

    TInt WaitNotification (TRequestStatus* aStat, TAny* aName , TInt aChoice);

   void Flush();
	
	TBool ValidCancellation(TInt aReqType);
public:

	void CancelRequest (TInt aReqType);
	
	void DoRequestCallback();
	
private:
	/////// Accessed within pipe mutex ///////
	
	DThread *iRequestThread; ///< The thread awaiting notification.
	TClientRequest* iClientRequest;

	//Allows us to tell if a request cancellation is valid
	TInt iRequestType; ///< Access within Pipe Mutex
	
	//////////////////////////////////////////


	// Reference to  the DPipe
	DPipe* iData;
	
	//Effectively constant
	RPipe::TChannelType iChannelType;
};



class DPipe:public DBase
/**
This class represent the actual Kernel side Pipe. An instance of this class is constructed 
when ever user creates a named/un-named pipe through the methods provided by user handler RPipe. 
The owner of a DPipe instance is the DPipeDevice factory object and associates this DPipe 
instance to the appropriate DPipeChannel instance. Each DPipe object is associated with two 
DPipeChannel instances for read and writes operation

@internalTechnology
*/
{
	friend class DPipeChannel;
	friend class DPipeDevice;
public:
	
	virtual ~DPipe();

	// Creates a Named pipe
	static DPipe* CreatePipe(const TDesC& aName, TInt aSize, TAny* aPolicy = NULL);

	// check if the name referring  to a created pipe is valid.
	TBool MatchName(const TDesC* aName);

	// Check if the id referring to a created pipe is valid.
	TBool MatchId(const TInt aId);


	// Check if Buffer is Empty
	TInt IsBufferEmpty();

	// Write to Buffer
	TInt Write(TAny* aBuf, TInt aSize);

	// Read to Buffer
	TInt Read(TAny* aBuf, TInt aSize);
	
	void SetReadEnd(DPipeChannel * aChannel);
	
	void SetWriteEnd(DPipeChannel * aChannel);


	// Registering Notification from client thread
	TInt RegisterSpaceAvailableNotification(TInt aSize);
	
	TInt RegisterDataAvailableNotification();
	
	TInt RegisterWaitNotification(TInt aChoice);	

	//! Cancellation methods
	void CancelSpaceAvailable();
	
	void CancelDataAvailable();
	
	void CancelWaitNotifier();
	
	TInt CloseReadEnd();
	
	TInt CloseWriteEnd();
	
	void CloseAll();

	TBool IsNamedPipe();
	
	TBool IsPipeClosed();
	
	TBool IsReadEndOpened();
	
	TBool IsWriteEndOpened();
	
	TInt OpenId();
	
	TInt Size();
	
	void SetId(TInt aId);
	
	void FlushPipe();
	
	TInt AvailableDataCount();
	
	inline TSecurityPolicy* GetCap(){ return &iPolicy;}

	
private:
	TInt ConstructPipe(const TDesC& aName, TInt aSize,TAny* aPolicy = NULL);
	
	inline void Wait()
		{
		Kern::MutexWait(*iPipeMutex);
		DATAPAGING_TEST(Kern::SetRealtimeState(ERealtimeStateOn);)
		}
	
	inline void Signal()
		{
		DATAPAGING_TEST(Kern::SetRealtimeState(ERealtimeStateOff);)
		Kern::MutexSignal(*iPipeMutex);
		}

	void MaybeCompleteSpaceNotification();

	inline DMutex& Mutex()
		{
		return *iPipeMutex;
		}

private:

	//! constructor
	DPipeChannel *iReadChannel;
	
	DPipeChannel *iWriteChannel;
	
	TKName iName;  //! TBuf<KMaxKernelName> TKName
	
	TInt iID;	

	//! Members for Ring buffer
	TInt iSize;
	
	TBool iFull;
	
	TUint8 *iBuffer;
	
	TInt iWritePointer;
	
	TInt iReadPointer;

	//! Signify the presence of read and write channel
	
	TBool iSpaceAvailableRequest;
	
	TBool iDataAvailableRequest;
	
	TBool iWaitRequest;
	
	TInt  iSpaceAvailableSize;
	
	DMutex *iPipeMutex;
	DMutex *iReadMutex;
	DMutex *iWriteMutex;

	
	TSecurityPolicy iPolicy;
	
};

/**
Acquire the given lock on construction and release
on destruction.
@internalTechnology
*/
template<typename T>
class TAutoWait
	{
public:
	inline TAutoWait(T& aLock)
		:iLock(aLock)
		{
		Wait();
		}

	inline ~TAutoWait()
		{
		Signal();
		}


private:
	TAutoWait(TAutoWait&);
	TAutoWait& operator= (TAutoWait&);

	//disallow allocating on the heap since
	//this won't do what we want
	void* operator new(TUint aSize);

	inline void Wait();
	inline void Signal();

	T& iLock;
	};

template<>
void TAutoWait<DMutex>::Wait()
	{
	NKern::ThreadEnterCS();
	Kern::MutexWait(iLock);
	}

template<>
void TAutoWait<DMutex>::Signal()
	{
	Kern::MutexSignal(iLock);
	NKern::ThreadLeaveCS();
	}

#endif




