// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\resource.h
//
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __RESOURCE_H__
#define __RESOURCE_H__
#include <kernel/kernel.h>
#include <kernel/kern_priv.h>
#include <drivers/resourcecontrol_trace.h>
#include <drivers/resource_category.h>

//Definition for resource flag setting. Used by PSL.
const TUint KTypeMask             = 0x3;
const TUint KUsageOffset          = 0x1F;
const TUint KLongLatencySetOffset = 0x1E;
const TUint KLongLatencyGetOffset = 0x1D;
const TUint KClassOffset          = 0x1C;
const TUint KSenseOffset          = 0x1A;
const TUint KShared               = 0x1u << KUsageOffset;
const TUint KLongLatencySet       = 0x1u << KLongLatencySetOffset;
const TUint KLongLatencyGet       = 0x1u << KLongLatencyGetOffset;
const TUint KLogical              = 0x1u << KClassOffset;
const TUint KSenseNegative        = 0x1u << KSenseOffset;
const TUint KSenseCustom          = 0x2u << KSenseOffset;

struct TPowerRequest;
struct SIdleResourceInfo;


/**
 * List of operations that a Custom Function may be informed of. 
 *
 * @publishedPartner
 * @prototype 9.5
 */
enum TCustomOperation
    {
	/**
	Client has requested the given level.
	*/
	EClientRequestLevel,

	/**
	Client has relinquished the given level.
    */
	EClientRelinquishLevel,
	
	/**
	Client is changing the level.
	*/
	EClientChangeLevel,
	
	/**
	A dynamic resource is being deregistered.
	*/
	EDynamicResourceDeregister
	};


/**
 *  Function prototype for the Custom Function.
 *  
 *  @publishedPartner
 *  @prototype 9.5
 */
typedef TBool (*TCustomFunction) (TInt& /*aClientId*/,
								  const TDesC8& /*aClientName*/,
                                  TUint /*aResourceId*/,
                                  TCustomOperation /*aCustomOperation*/,
                                  TInt& /*aLevel*/,
                                  TAny* /*aLevelList*/,
                                  TAny* /*aReserved*/);  // For future use

/**
@publishedPartner
@prototype 9.5
class to represent static resources
*/
class DStaticPowerResource : public DBase
	{
public:
    enum TType {EBinary = EResBinary, EMultilevel, EMultiProperty};
    enum TUsage {ESingleUse = EResSingleUse, EShared};
    enum TLatency {EInstantaneous = EResInstantaneous, ELongLatency};
    enum TClass {EPhysical = EResPhysical, ELogical};
    enum TSense {EPositive = EResPositive, ENegative, ECustom};
public:
    //exported to allow construction from other base port components.
    IMPORT_C DStaticPowerResource(const TDesC8& aName, TInt aDefaultLevel);
    IMPORT_C virtual TInt GetInfo(TDes8* aInfo)const;
    inline void SetCustomFunction(TCustomFunction aCustomFunction)
           {iCustomFunction=aCustomFunction;}

    //private data inlined accessors
    TType Type() const {return TType(iFlags&KTypeMask);}
    TClass Class() const {return TClass((iFlags>>KClassOffset)&0x1);}
    TUsage Usage() const {return TUsage((iFlags>>KUsageOffset)&0x1);}
    TLatency LatencyGet() const
             {return TLatency((iFlags>>KLongLatencyGetOffset)&0x1);}
    TLatency LatencySet() const
             {return TLatency((iFlags>>KLongLatencySetOffset)&0x1);}
    TSense Sense() const {return TSense((iFlags>>KSenseOffset)&0x3);}
protected:
    //pure virtual function to be implement by PSL
    virtual TInt DoRequest(TPowerRequest& aRequest)=0;
public:
	HBuf8* iName;
protected:
    TInt iPostBootLevel;
	TInt iDefaultLevel;
protected:
    TInt iCachedLevel;
	TUint iResourceId;
    SDblQue iNotificationList;
    SDblQue iClientList;
    TCustomFunction iCustomFunction;
    TInt iLevelOwnerId;
    SIdleResourceInfo* iIdleListEntry;
protected:
    TUint iFlags;
#ifdef PRM_CONTROLLER
    friend class DPowerResourceController;
#endif
	};

typedef void (*TPowerResourceCbFn)(TUint /*aClientId*/,
                                   TUint /*aResourceId*/,
                                   TInt /*aLevel*/,
								   TInt /*aLevelOwnerId*/,
                                   TInt /*aResult*/,
                                   TAny* /*aParam*/);

/**
@publishedPartner
@prototype 9.5
An object of this type represents a customized Dfc
used to signal completion of the resource manager's asynchronous APIs
and completion of notifications
@see TPowerResourceManager
 */
class TPowerResourceCb : public TDfc
	{
public:
	inline TPowerResourceCb(TPowerResourceCbFn aFn, TAny* aPtr, TInt aPriority) : TDfc(DfcFunc, this, aPriority), 
		                                                            iParam(aPtr), iCallback(aFn){ }
	inline TPowerResourceCb(TPowerResourceCbFn aFn, TAny* aPtr, TDfcQue* aQue, TInt aPriority):  
	                              TDfc(DfcFunc, this, aQue, aPriority), iParam(aPtr), iCallback(aFn) { }
private:
    inline static void DfcFunc(TAny* aPtr)
        {
        TPowerResourceCb* pCb = (TPowerResourceCb*) aPtr;
	    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">TPowerResourceCb::DfcFunc ClientId = 0x%x, ResourceId = %d, Level = %d, \
			                    LevelOwnerId = %d, Result = %d", pCb->iClientId, pCb->iResourceId, pCb->iLevel, \
								pCb->iLevelOwnerId, pCb->iResult));

        pCb->Lock();
        TUint ClientId = pCb->iClientId;
        TUint ResourceId = pCb->iResourceId;
        TInt Level = pCb->iLevel;
        TInt LevelOwnerId = pCb->iLevelOwnerId;
        TInt Result = pCb->iResult;
        TAny* Param = pCb->iParam;
        pCb->UnLock();

        // Call the client specified callback function
        pCb->iCallback(ClientId, ResourceId, Level, LevelOwnerId, Result, Param);    

        pCb->Lock();
        pCb->iPendingRequestCount--;
        if(pCb->iPendingRequestCount == 0)
            pCb->iResult = KErrCompletion; //Mark the callback object to act properly during cancellation of this request.
        pCb->UnLock();
        PRM_CALLBACK_COMPLETION_TRACE
        }
private:
    void Lock()
        {
		__ASSERT_DEBUG(iMutex, Kern::Fault("TPowerResourceCb::Lock", __LINE__));
        NKern::ThreadEnterCS();
        Kern::MutexWait(*iMutex);
        }
    void UnLock()
        {
		__ASSERT_DEBUG(iMutex, Kern::Fault("TPowerResourceCb::UnLock", __LINE__));
        Kern::MutexSignal(*iMutex);
        NKern::ThreadLeaveCS();
        }
    TAny* iParam; //Stores the aPtr argument passed in the constructor, to be passed as 5th argument to the callback function
    TInt iResult; //Used to store the result as well as binary usage count for the callback
    TInt iLevel; // Level of the resource
	TInt iLevelOwnerId; // Stores owner of the resource for asynchronous get operation
    TUint iResourceId; //Stores the ID of the resource whose state is changed/read asynchronously
    TUint iClientId; //Stores the ID of the client that requested the asynchronous operation
    TPowerResourceCbFn iCallback; //Callback function object
	DMutex* iMutex;   
    TInt iPendingRequestCount;
#ifdef PRM_CONTROLLER
    friend class DPowerResourceController;
#endif
	};

/**
@publishedPartner
@prototype 9.5
Notifications class. Conditional and unconditional notifications are encapsulated in this class. 
It uses TPowerResourceCb to perform the actual notification call.
@see TPowerResourceCb
 */
class DPowerResourceNotification : public DBase
    {
public:
    enum TType {EUnconditional, EConditional};
	enum TResNotiPanic {ENotificationObjectStillInList = 25}; 
public:
	inline DPowerResourceNotification(TPowerResourceCbFn aFn, TAny* aPtr, TInt aPriority): 
	                                                 iCallback(aFn, aPtr, aPriority) {}
	inline DPowerResourceNotification(TPowerResourceCbFn aFn, TAny* aPtr, TDfcQue* aQue, TInt aPriority) : 
	                                                 iCallback(aFn, aPtr, aQue, aPriority) {}
	inline ~DPowerResourceNotification() 
		{
		if(iRegistered)
			Kern::Fault("Power Resource Controller", ENotificationObjectStillInList);
		}
public:
	TInt iPreviousLevel; //Previous level of the resource. This is used for checking the threshold condition
	TUint8 iRegistered;
private:
    TUint8 iType; // the type of notification required (conditional or unconditional).
    TUint16 iOwnerId; // the bottom 16 bits of the Id of the client which requested the notification
    TInt iThreshold; // the threshold which when crossed on a specified direction will cause a notification to be issued.
    TBool iDirection; // the direction of the resource change that together with the threshold to be crossed will result in the notification
    TPowerResourceCb iCallback; //Callback object associated with this notification
    SDblQueLink iNotificationLink; 
    DPowerResourceNotification* iNextInClient;
#ifdef PRM_CONTROLLER
    friend class DPowerResourceController;
#endif
	};

/**
@publishedPartner
@prototype 9.5
class to represent resource properties
*/
class TPowerResourceInfoV01
	{
public:
    DStaticPowerResource::TClass iClass;
    DStaticPowerResource::TLatency iLatencyGet;
    DStaticPowerResource::TLatency iLatencySet;
    DStaticPowerResource::TType iType;
    DStaticPowerResource::TUsage iUsage;
    DStaticPowerResource::TSense iSense;
    TDesC8* iResourceName;
    TUint iResourceId;
    TInt iDefaultLevel;  
    TInt iMinLevel;      //PSL mandatory field
    TInt iMaxLevel;      //PSL mandatory field
	TInt iReserved1;	 //Reserved for future use.	
	TInt iReserved2;	 //Reserved for future use.
	TInt iReserved3;	 //Reserved for future use.
    TInt iPslReserved1;  //PSL specific field
    TInt iPslReserved2;  //PSL specific field
    TInt iPslReserved3;  //PSL specific field
	};

/**
@publishedPartner
@prototype 9.5
*/
typedef TPckgBuf<TPowerResourceInfoV01> TPowerResourceInfoBuf01;

/**
@publishedPartner
@prototype 9.5
structure to represent client properties
*/
struct TPowerClientInfoV01
	{
    TUint iClientId;
    TDesC8* iClientName;
	};

#endif // __RESOURCE_H__
