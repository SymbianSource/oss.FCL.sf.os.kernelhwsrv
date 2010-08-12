/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
// Resmanus.h
//
//

// Base classes for implementating power resource support (Kernel-side only)

/**
@file
@internalComponent
*/
#ifndef __RESMANUS_H__
#define __RESMANUS_H__

#include <platform.h>
#include <d32resmanus.h>

#define USERSIDE_LDD
#include <drivers/resourcecontrol.h>	// For class DResourceController

#include <e32ver.h>
#include <e32des8.h>	// for HBufC8

// Use the following macro for debug output of request-tracking information
// #define _DUMP_TRACKERS

const TInt KResManUsMajorVersionNumber = 1;
const TInt KResManUsMinorVersionNumber = 0;
const TInt KResManUsBuildVersionNumber = KE32BuildVersionNumber;

const TInt KMaxNumChannels = 4;	// Limit for the number of channels to be open

const TInt KNumClientNamesResCtrl	= 10; // Default number for kernel plus user side clients
const TInt KNumClientNamesUserSide	= 10; // Sized as above (client may have PlatSec capabilities for access)
const TInt KNumResourceInfoResCtrl	= 25; // To contain object types used by Resource Controller
const TInt KNumResourceInfoUserSide	= 25; // To contain object types used by Client
const TInt KNumResourceDependencies = 5; // Default number of dependencies for a resource

const TUint8 KAllResInfoStored = 0xff;
const TUint8 KAllClientInfoStored = 0xff;

enum TAsyncOpType
	{
	EGetState,
	ESetState,
	ENotify
	};

#define USER_SIDE_CLIENT_BIT_MASK 0x4000 //Bit 14


class DChannelResManUs;

/*
	Classes used to track client usage
*/

class TTrackingControl
	{
	public:
	DChannelResManUs* iOwningChannel;
	SDblQue* iFreeQue;
	SDblQue* iBusyQue;
	TAsyncOpType iType;
	TUint8 iReserved1; // reserved for future expansion
	TUint8 iReserved2; // reserved for future expansion
	TUint8 iReserved3; // reserved for future expansion
	};

class TTrackingBuffer : public SDblQueLink
	{
	public:
	inline void SetTrackingControl(TTrackingControl* aControl){iTrackingControl=aControl;};
	inline TTrackingControl* GetTrackingControl(){return iTrackingControl;};
	inline TUint GetResourceId(){return iResourceId;};
	inline void SetResourceId(TUint aResourceId){iResourceId=aResourceId;};
	inline SDblQue* GetQue() {return iQue;};
	inline void SetQue(SDblQue* aQue) {iQue=aQue;};

	private:
	TTrackingControl* iTrackingControl;
	SDblQue* iQue;
	TUint iResourceId;
	};

class TTrackGetStateBuf : public TTrackingBuffer
	{
	public:
	TTrackGetStateBuf(TPowerResourceCbFn aFn, TAny* aPtr, 
					TDfcQue* aQue, TInt aPriority);
	~TTrackGetStateBuf();

	public:
	TPowerResourceCb iCtrlBlock;
	TClientDataRequest2<TInt,TInt>* iRequest;
	};

class TTrackSetStateBuf : public TTrackingBuffer
	{
	public:
	TTrackSetStateBuf(TPowerResourceCbFn aFn, TAny* aPtr, 
					TDfcQue* aQue, TInt aPriority);
	~TTrackSetStateBuf();
 	public:
	TPowerResourceCb iCtrlBlock;
	TClientRequest* iRequest;
	};

class TTrackNotifyBuf : public TTrackingBuffer
	{
	public:
	TTrackNotifyBuf(TPowerResourceCbFn aFn, TAny* aPtr, 
					TDfcQue* aQue, TInt aPriority);
	~TTrackNotifyBuf();
	public:
	DPowerResourceNotification iNotifyBlock;
	TClientRequest* iRequest;
	};


/* 
	Power resource logical device
	The class representing the power resource logical device
*/
class DDeviceResManUs : public DLogicalDevice
    {
    public:
    /**
     * The constructor
     */
    DDeviceResManUs();
    /**
     * The destructor
     */
    ~DDeviceResManUs();
    /**
     * Second stage constructor - install the device
     */
    virtual TInt Install();
    /**
     * Get the Capabilites of the device
     * @param aDes descriptor that will contain the returned capibilites
     */
    virtual void GetCaps(TDes8 &aDes) const;
    /**
     * Create a logical channel to the device
     */
    virtual TInt Create(DLogicalChannelBase*& aChannel);

	public:
#ifndef RESOURCE_MANAGER_SIMULATED_PSL
	TDfcQue* iSharedDfcQue; // To allow access from device entry point
#else
	TDynamicDfcQue* iSharedDfcQue; // To allow LDD unload/re-load in testing
#endif
    };



	// The logical channel for power resource devices
class DChannelResManUs : public DLogicalChannel
    {
    public:

	/*
     * The constructor 
     */
    DChannelResManUs();
	/*   
     * The destructor
     */
    ~DChannelResManUs();

	// Helper methods
	TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
	void FreeTrackingBuffer(TTrackingBuffer*& aBuffer);

	inline TInt ClientHandle() {return iClientHandle;};

    /**
     * Create a logical power resource channel
     * @param aUnit The channel number to create
     * @param anInfo not used, can be NULL
     * @param aVer The minimun driver version allowed
     * @return KErrNone if channel created
     */
    virtual TInt DoCreate(TInt aUnit, const TDesC8* aInfo, const TVersion& aVer);

    protected:
    /**
     * Handle a message from the channels user
     * @param aMsg The message to handle
     */
    virtual void HandleMsg(TMessageBase* aMsg);	// Note: this is a pure virtual in DLogicalChannel

	virtual TInt SendMsg(TMessageBase* aMsg);

	TInt SendControl(TMessageBase* aMsg);

	TInt SendRequest(TMessageBase* aMsg);
    /**
     * Cancel an outstanding request
     * @param aMask A mask containing the requests to be canceled
     */
    void DoCancel(TInt aMask);	// Name for convenience!
    /**
     * Preform a control operation on the channel
     * Control operations are:
     * - Get the current configuration
     * - Configure the channel
     * - Set the MAC address for the channel
     * - Get the capibilities of the channel
     * @param aId The operation to preform
     * @param a1 The data to use with the operation
     * @param a2 can be NULL - not used
     * @return KErrNone if operation done
     */
   TInt DoControl(TInt aId, TAny* a1, TAny* a2); // Name for convenience!
    /**
     * Preform an asynchros operation on the channel
     * Operations are:
     * - Read data from the channel
     * - Write data to the channel
     * @param aId The operation to perform
     * @param aStatus The status object to use when complete
     * @param a1 The data to use
     * @param a2 The length of the data to use
     * @return KErrNone if operation started ok
     * @see Complete()
     */
    TInt DoRequest(TInt aId, TRequestStatus* aStatus, TAny* a1, TAny* a2); // Name for convenience!


	inline void SetClientHandle(TInt aHandle) {iClientHandle=aHandle;};

	TInt InitTrackingControl(TTrackingControl*& aTracker, TUint8 aType, TUint8 aNumBuffers);
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
	void GetNumCandidateAsyncResources(TUint& aNumResources);
	TInt GetCandidateAsyncResourceId(TUint aIndex, TUint& aResourceId);
	void GetNumCandidateSharedResources(TUint& aNumResources);
	TInt GetCandidateSharedResourceId(TUint aIndex, TUint& aResourceId);
#endif

    private:
	static void RegistrationDfcFunc(TAny* aChannel);
	TInt RegisterWithResCtrlr();
	TInt GetValidName(const TDesC8* aInfo);
	void RemoveTrackingControl(TTrackingControl*& aTracker);
	TInt GetAndInitTrackingBuffer(TTrackingControl*& aTracker, TTrackingBuffer*& aBuffer, TUint aResourceId, TRequestStatus* aStatus);
	TInt GetStateBuffer(TTrackingControl*& aTracker, TTrackingBuffer*& aBuffer, TUint aResourceId, TInt *aState, TInt* aLevelOwnerPtr, TPowerResourceCb*& aCb, TRequestStatus* aStatus);
	TTrackingControl* MapRequestToTracker(TInt aRequestType);
	TInt CancelTrackerRequests(TTrackingControl* aTracker,TBool aSingleRsrc, TUint aResourceId, TRequestStatus* aStatus);
	TInt CancelRequestsOfType(TInt aRequestType, TRequestStatus* aStatus);
	TInt EnsureSizeIsSufficient(HBuf*& aBuffer, TInt aMinSize);
	TInt ExtractResourceInfo(const TPowerResourceInfoV01* aPwrResInfo, TResourceInfoBuf& aInfo);
#ifdef _DUMP_TRACKERS
	TInt DumpResource(const TPowerResourceInfoV01* aResource);
	void DumpTracker(TTrackingControl* aTracker);
#endif
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
	void CheckForCandidateAsyncResource(TPowerResourceInfoV01* aResource);
	void CheckForCandidateSharedResource(TPowerResourceInfoV01* aResource);
#endif
	typedef void ClientCopyFunc(TDes8*, const TPowerClientInfoV01*);

	// Registration and identification support
    public:
	DThread* iClient;

	DPowerResourceController* iPddPtr;

	private:
	NFastMutex iBufferFastMutex;
	NFastSemaphore *iFastSem;
	TInt iClientHandle;
	TUint iNameProvidedLength;
	HBuf8* iUserNameUsed;

	// Support for usage tracking
	TTrackingControl *iGetStateTracker;
	TTrackingControl *iSetStateTracker;
	TTrackingControl *iListenableTracker;

	// Buffers to support acquisition of resource and client information
	HBuf8* iClientNamesResCtrl;		// Stores client information
	TUint iClientInfoStoredResId;	// The ID of the resource for which the data is stored (none=0, all=KAllClientInfoStored)
	TUint iClientInfoStoredNum;		// The number of clients for which data is stored

	HBuf8* iResourceInfoResCtrl;	// Stores resource information
	TUint iResInfoStoredClientId;	// The ID of the client for which the data is stored (none=0, all=KAllResInfoStored)
	TUint iResInfoStoredNum;		// The number of resources for which data is stored

	HBuf8* iResourceDependencyIds;	// To contain the identifiers for resource dependencies
	TUint iNumResDepsStored;

#ifdef RESOURCE_MANAGER_SIMULATED_PSL
	// Support for testing
	TBool iHaveLongLatencyResource;

	// Array for candidate resources to use for testing
	// Store a maximum of MAX_NUM_CANDIDATE_RESOURCES
	#define MAX_NUM_CANDIDATE_RESOURCES 10
	TUint iNoCandidateAsyncRes;
	TUint iCandidateAsyncResIds[MAX_NUM_CANDIDATE_RESOURCES];
	TUint iNoCandidateSharedRes;
	TUint iCandidateSharedResIds[MAX_NUM_CANDIDATE_RESOURCES];
#endif

	// 8-bit values, placed here to aid size management
	TUint8 iClientInfoValid;		// To indicate if a valid set of client data is stored
	TUint8 iResInfoValid;			// To indicate if a valid set of resource data is stored
	TUint8 iResDepsValid;			// Guard flag for the RArray

	TUint8 iReserved1;				 // reserved for future expansion

    };


#endif
