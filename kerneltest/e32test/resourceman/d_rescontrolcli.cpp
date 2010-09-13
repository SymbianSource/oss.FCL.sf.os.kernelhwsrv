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
// e32test\resourceman\d_rescontrol_cli.cpp
// 
//

#include <kernel/kern_priv.h>
#include <drivers/resource_extend.h>
#include <drivers/resourceman.h>
#include "d_rescontrolcli.h"
#ifdef PRM_ENABLE_EXTENDED_VERSION
#include "dynamicresource.h"
#endif
#include "./resourceman_psl/rescontrol_psl.h"

#ifndef PRM_ENABLE_EXTENDED_VERSION
_LIT(KTestPowerRCName, "D_RESCONTROLCLI.LDD");
#else
_LIT(KTestPowerRCName, "D_EXTENDEDRESCONTROLCLI.LDD");
#endif

const TInt KTestResManLddThreadPriority  = 0x5;
_LIT(KTestResManLddThread, "TestResManLddThread");

#ifdef PRM_ENABLE_EXTENDED_VERSION
#define EXPECTED_POST_NOTI_COUNT 6 //Expected number of notifications as a result of post boot level setting. 
#else
#define EXPECTED_POST_NOTI_COUNT 5
#endif
#define MAX_DYNAMIC_RES_NUM 5
/** 
Structure for holding resource information
*/
struct SClientInfo
	{
	HBuf8* pName;
	TUint iClientId;
	};
/**
The logical device (factory class) for the resource manager client side test.
*/
class DTestResManLddFactory : public DLogicalDevice
	{
public:
	DTestResManLddFactory();
	~DTestResManLddFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	static void PostBootNotificationFunc(TUint aClientId, TUint aResId, TInt aLevel, TInt aLevelOwnerId, TInt aResult, TAny* aParam);
	void SetPostBootLevelAndRequestNotification(TUint aClientId, TUint aResId, TInt aPostBootLevel);
	TDynamicDfcQue* iDfcQ;
	DStaticPowerResource *iStaticRes;
	DStaticPowerResource *iStaticResArray[3];
	static SClientInfo iClient; //Need to be static to access them in PostBootNotificationFunc (callback function).
	static TUint iPostBootNotiCount;
	};

/** Logical channel class for Resource manager test LDD */
class DTestResManLdd : public DLogicalChannel
	{
public:
	DTestResManLdd();
	virtual ~DTestResManLdd();
	// Inherited from DLogicalChannel
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual void HandleMsg(TMessageBase* aMsg);
private:
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	TInt DoRequest(TInt aFunction, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	TInt DoCancel(TUint aMask);
	static void CallbackFunc(TUint aClientId, TUint aResId, TInt aLevel, TInt aLevelOwnerId, TInt aResult, TAny* aParam);
	static void CondNotificationFunc(TUint aClientId, TUint aResId, TInt aLevel, TInt aLevelOwnerId, TInt aResult, TAny* aParam);
	static void UnCondNotificationFunc(TUint aClientId, TUint aResId, TInt aLevel, TInt aLevelOwnerId, TInt aResult, TAny* aParam);
private:
   	DThread* iClientThreadPtr;
   	TPowerResourceCb iAsyncResourceCallback; //Callback object for handling long latency resources.
	//Structure for storing the notification information
	struct SNotificationList
		{
		TUint iClientId;
		TUint iResourceId;
		TUint iCount;
		DPowerResourceNotification *iNoti;
		SNotificationList *iNext;
		};
	SNotificationList *iCondList; //List for maintaining conditional notification
	SNotificationList *iUnCondList; //List for maintaining unconditional notification
	TRequestStatus *iStatus; //Store the status and complete on callback function.
	TBool iCallbackCancel; 
	TInt iClientId; //ID of the client that requested long latency operation, to compare in callback and notifications.
	TUint iResourceId; //Id of the resource of long latency operation, to compare in callback and notifications.
	//Below 2 variables are used only for asynchronous get resource state operation
	TInt *iStatePtr; //Pointer to hold the address of state variable, where state is updated in callback function.
	TInt *iLevelOwnerIdPtr; //Pointer to hold the address of level owner Id variable, where owner Id is updated in callback function.
	HBuf *pBuf; //Buffer for testing caching of resource information for Idle power management 
	SClientInfo clientInfo[MAX_CLIENTS];
#ifdef PRM_ENABLE_EXTENDED_VERSION
	TUint iTestParallelResourceId; //Variable to hold the long latency resource id, this is used only in test parallel execution of DFC's
	static void TestParallelExecutionCallback(TUint aClientId, TUint aResId, TInt aLevel, TInt aLevelOwnerId, TInt aResult, TAny* aParam);
	TBool iCallbackReceived;
	TBool iValidateCallbackReceived;
	TPowerResourceCb iAsyncTestParallelCallback; //Callback object for handling long latency resources change state while deregistering non-dependency resource.
	RArray <DDynamicPowerResource*> iDynamicArray; //Array to hold dynamic resources. This includes dynamic dependent resource
#endif
	};

//class definition for multilevel single instantaneous positive resource
class DLaterRegisterStaticResource : public DStaticPowerResource
	{
public:
	DLaterRegisterStaticResource();
	TInt DoRequest(TPowerRequest &req);
	TInt GetInfo(TDes8* aInfo) const;
private:
	TInt iMinLevel;
	TInt iMaxLevel;
	TInt iCurrentLevel;
	};

//Constructors of the resource
_LIT(KLaterRegisterStaticResource, "LaterRegisterStaticResource");
DLaterRegisterStaticResource::DLaterRegisterStaticResource() : DStaticPowerResource(KLaterRegisterStaticResource, E_OFF), iMinLevel(0), iMaxLevel(1), iCurrentLevel(0)
	{
	iFlags = KBinary;
	}

TInt DLaterRegisterStaticResource::DoRequest(TPowerRequest& req)
	{
	Kern::Printf("DLaterRegisterStaticResource::DoRequest\n");
	if(req.ReqType() == TPowerRequest::EGet)
		{
		req.Level() = iCurrentLevel;
		}
	else if(req.ReqType() == TPowerRequest::EChange)
		{
		iCurrentLevel = req.Level();
		}
	else if(req.ReqType() == TPowerRequest::ESetDefaultLevel)
		{
		req.Level() = iDefaultLevel;
		iCurrentLevel = iDefaultLevel;
		}
	else
		return KErrNotSupported;
	return KErrNone;
	}

TInt DLaterRegisterStaticResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iDefaultLevel = iDefaultLevel;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

//class definition for multilevel single instantaneous positive resource
class DLaterRegisterStaticResource1 : public DStaticPowerResource
	{
public:
	DLaterRegisterStaticResource1();
	TInt DoRequest(TPowerRequest &req);
	TInt GetInfo(TDes8* aInfo) const;
private:
	TInt iMinLevel;
	TInt iMaxLevel;
	TInt iCurrentLevel;
	};

//Constructors of the resource
_LIT(KLaterRegisterStaticResource1, "LaterRegisterStaticResource1");
DLaterRegisterStaticResource1::DLaterRegisterStaticResource1() : DStaticPowerResource(KLaterRegisterStaticResource1, E_OFF), iMinLevel(0), iMaxLevel(1), iCurrentLevel(0)
	{
	iFlags = KBinary;
	}

TInt DLaterRegisterStaticResource1::DoRequest(TPowerRequest& req)
	{
	Kern::Printf("DLaterRegisterStaticResource1::DoRequest\n");
	if(req.ReqType() == TPowerRequest::EGet)
		{
		req.Level() = iCurrentLevel;
		}
	else if(req.ReqType() == TPowerRequest::EChange)
		{
		iCurrentLevel = req.Level();
		}
	else if(req.ReqType() == TPowerRequest::ESetDefaultLevel)
		{
		req.Level() = iDefaultLevel;
		iCurrentLevel = iDefaultLevel;
		}
	else
		return KErrNotSupported;
	return KErrNone;
	}

TInt DLaterRegisterStaticResource1::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iDefaultLevel = iDefaultLevel;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

//class definition for multilevel single instantaneous positive resource
class DLaterRegisterStaticResource2 : public DStaticPowerResource
	{
public:
	DLaterRegisterStaticResource2();
	TInt DoRequest(TPowerRequest &req);
	TInt GetInfo(TDes8* aInfo) const;
private:
	TInt iMinLevel;
	TInt iMaxLevel;
	TInt iCurrentLevel;
	};

//Constructors of the resource
_LIT(KLaterRegisterStaticResource2, "LaterRegisterStaticResource2");
DLaterRegisterStaticResource2::DLaterRegisterStaticResource2() : DStaticPowerResource(KLaterRegisterStaticResource2, E_OFF), iMinLevel(0), iMaxLevel(1), iCurrentLevel(0)
	{
	iFlags = KBinary;
	}

TInt DLaterRegisterStaticResource2::DoRequest(TPowerRequest& req)
	{
	Kern::Printf("DLaterRegisterStaticResource2::DoRequest\n");
	if(req.ReqType() == TPowerRequest::EGet)
		{
		req.Level() = iCurrentLevel;
		}
	else if(req.ReqType() == TPowerRequest::EChange)
		{
		iCurrentLevel = req.Level();
		}
	else if(req.ReqType() == TPowerRequest::ESetDefaultLevel)
		{
		req.Level() = iDefaultLevel;
		iCurrentLevel = iDefaultLevel;
		}
	else
		return KErrNotSupported;
	return KErrNone;
	}

TInt DLaterRegisterStaticResource2::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iDefaultLevel = iDefaultLevel;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TUint DTestResManLddFactory::iPostBootNotiCount = 0;
SClientInfo DTestResManLddFactory::iClient = {NULL, 0};
DTestResManLddFactory::DTestResManLddFactory()
	{
	iParseMask=0; // Allow info and pdd, but not units
	iUnitsMask=0;
	// Set version number for this device
	iVersion=RTestResMan::VersionRequired();
	}

DTestResManLddFactory::~DTestResManLddFactory()
	{
	if(iDfcQ)
	  iDfcQ->Destroy();
	}

/** Entry point for this driver */
DECLARE_STANDARD_LDD()
	{
	DTestResManLddFactory* p = new DTestResManLddFactory;
	if(!p)
		return NULL;
	TInt r = Kern::DynamicDfcQCreate(p->iDfcQ, KTestResManLddThreadPriority, KTestResManLddThread);
	if(r != KErrNone)
		{
		Kern::Printf("Memory not allocated");
		p->AsyncDelete();
		return NULL;
		}

#ifdef CPU_AFFINITY_ANY
		NKern::ThreadSetCpuAffinity((NThread*)(p->iDfcQ->iThread), KCpuAffinityAny);			
#endif

	//Register client with Resource Controller
	TBuf8<32> ClientName(_L8("Client"));
	p->iClient.pName = HBuf::New((const TDesC&)ClientName);
	if(!p->iClient.pName)
		{
		p->iDfcQ->Destroy();
		p->AsyncDelete();
		return NULL;
		}
	//Allocating memory earlier so that during failure conditions can cleanup easily
	p->iStaticRes = new DLaterRegisterStaticResource(); // it will be registered, and later destroyed by the ResourceManager
	if(!p->iStaticRes)
		{
		delete p->iClient.pName;
		p->iDfcQ->Destroy();
		p->AsyncDelete();
		return NULL;
		}
	p->iStaticResArray[0] = new DLaterRegisterStaticResource1(); // it will be registered, and later destroyed by the ResourceManager
	if(!p->iStaticResArray[0])
		{
		delete p->iStaticRes;
		delete p->iClient.pName;
		p->iDfcQ->Destroy();
		p->AsyncDelete();
		return NULL;
		}	
	p->iStaticResArray[2] = new DLaterRegisterStaticResource2();
	if(!p->iStaticResArray[2])
		{
		delete p->iStaticRes;
		delete p->iStaticResArray[0];
		delete p->iClient.pName;
		p->iDfcQ->Destroy();
		p->AsyncDelete();
		return NULL;
		}
	r = PowerResourceManager::RegisterClient(DTestResManLddFactory::iClient.iClientId, (const TDesC&)*DTestResManLddFactory::iClient.pName);
	if(r != KErrNone)
		{
		Kern::Printf("RegisterClient Failed\n");
		Kern::Fault("PRM REGISTER CLIENT FAILED", __LINE__);
		}
	//Set postbootlevel for these resources.
	p->SetPostBootLevelAndRequestNotification(DTestResManLddFactory::iClient.iClientId, 1, 0);
	p->SetPostBootLevelAndRequestNotification(DTestResManLddFactory::iClient.iClientId, 6, 12);
	p->SetPostBootLevelAndRequestNotification(DTestResManLddFactory::iClient.iClientId, 7, 1);
	p->SetPostBootLevelAndRequestNotification(DTestResManLddFactory::iClient.iClientId, 12, 10);
	p->SetPostBootLevelAndRequestNotification(DTestResManLddFactory::iClient.iClientId, 15, 0);
#ifdef PRM_ENABLE_EXTENDED_VERSION
	p->SetPostBootLevelAndRequestNotification(DTestResManLddFactory::iClient.iClientId, 65537, -50);
#endif
	//Test the later registration of static resources.
	r = DPowerResourceController::RegisterStaticResource(DTestResManLddFactory::iClient.iClientId, p->iStaticRes);
	if(r != KErrNone)
		Kern::Fault("PRM REGISTER STATIC RESOURCE FAILED", __LINE__);
	DStaticPowerResource **resPtr = &p->iStaticResArray[0];
	r = DPowerResourceController::RegisterArrayOfStaticResources(DTestResManLddFactory::iClient.iClientId, resPtr, 3);
	if(r != KErrNone)
		Kern::Fault("PRM REGISTER STATIC RESOURCE FAILED", __LINE__);
	r = DSimulatedPowerResourceController::CompleteResourceControllerInitialisation();
	if(r != KErrNone)
		Kern::Fault("PRM INIT FAILED", __LINE__);
	return p;
	}

void DTestResManLddFactory::SetPostBootLevelAndRequestNotification(TUint aClientId, TUint aResId, TInt aPostBootLevel)
	{
	DPowerResourceController::PostBootLevel(aResId, aPostBootLevel);
	DPowerResourceNotification* iNoti = new DPowerResourceNotification(PostBootNotificationFunc, (TAny*)NULL, Kern::DfcQue0(), 3);
	if(!iNoti)
		{
		Kern::Fault("PRM NOTI FAILED", __LINE__);
		}
	//Passing the address of the object so that it could be deleted in callback function
	new (iNoti) DPowerResourceNotification(PostBootNotificationFunc, iNoti, Kern::DfcQue0(), 3);
	TInt r = PowerResourceManager::RequestNotification(aClientId, aResId, *iNoti);
	if(r != KErrNone)
		Kern::Fault("PRM REQ NOTI FAILED", __LINE__);
	}


/** Second stage constuctor */
TInt DTestResManLddFactory::Install()
	{
   	return(SetName(&KTestPowerRCName));
	}

/** Device capabilities */
void DTestResManLddFactory::GetCaps(TDes8& aDes)const
	{
	// Create a capabilities object
	RTestResMan::TCaps caps;
	caps.iVersion = iVersion;
	// Write it back to user memory
	Kern::InfoCopy(aDes,(TUint8*)&caps,sizeof(caps));
	}

/** Create logical channel, only open of one channel is allowed */
TInt DTestResManLddFactory::Create(DLogicalChannelBase*& aChannel)
	{
   	if (iOpenChannels != 0) //A Channel is already open
		return KErrInUse;
	aChannel = new DTestResManLdd;
	if(!aChannel)
		return KErrNoMemory;
	return KErrNone;
	}

/** Constructor */
DTestResManLdd::DTestResManLdd(): iAsyncResourceCallback(CallbackFunc, this, 3)
#ifdef PRM_ENABLE_EXTENDED_VERSION
				, iAsyncTestParallelCallback(TestParallelExecutionCallback, this, 3)
#endif
	{
	iCondList = NULL;
	iUnCondList = NULL;
	iCallbackCancel = EFalse;
	iClientId = 0;
	iResourceId = 0;
	iStatePtr = NULL;
	iLevelOwnerIdPtr = NULL;
	for(TUint c = 0; c < MAX_CLIENTS; c++)
		{
		clientInfo[c].iClientId = 0;
		clientInfo[c].pName = NULL;
		}
	iClientThreadPtr=&Kern::CurrentThread();
	// Increase the DThread's ref count so that it does not close without us
	((DObject*)iClientThreadPtr)->Open();;
	}

/** Destructor */
DTestResManLdd::~DTestResManLdd()
	{
	if(pBuf) //Buffer created for storing idle resource information
		delete pBuf;
#ifdef PRM_ENABLE_EXTENDED_VERSION
	for(TInt c = 0; c < iDynamicArray.Count(); c++)
		{
		delete iDynamicArray[c]; //Delete the dynamic array. This includes dynamic dependent resource.
		}
	iDynamicArray.Close();
#endif
	// Close our reference on the client thread
	Kern::SafeClose((DObject*&)iClientThreadPtr,NULL);
	}

/** Second stage constructor. */
TInt DTestResManLdd::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& aVer)
	{
   	// Check version
	if (!Kern::QueryVersionSupported(RTestResMan::VersionRequired(),aVer))
		return KErrNotSupported;
	SetDfcQ(((DTestResManLddFactory*)iDevice)->iDfcQ);
	iAsyncResourceCallback.SetDfcQ(iDfcQ);
#ifdef PRM_ENABLE_EXTENDED_VERSION
	iAsyncTestParallelCallback.SetDfcQ(iDfcQ);
#endif
 	iMsgQ.Receive();
	return KErrNone;
	}

/** Process a message for this logical channel */
void DTestResManLdd::HandleMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m=*(TThreadMessage*)aMsg;
	TInt id=m.iValue;

	if (id==(TInt)ECloseMsg)
		{
		// Channel close.
		m.Complete(KErrNone,EFalse);
		return;
		}
	else if (id==KMaxTInt)
		{
		// DoCancel
		m.Complete(KErrNone,ETrue);
		return;
		}
	else if (id<0)
		{
		// DoRequest
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		TInt r=DoRequest(~id,pS,m.Ptr1(),m.Ptr2());
		if (r!=KErrNone)
			Kern::RequestComplete(iClientThreadPtr,pS,r);
		m.Complete(KErrNone,ETrue);
		}
	else
		{
		// DoControl
		TInt r=DoControl(id,m.Ptr0(),m.Ptr1());
		m.Complete(r,ETrue);
		}
	}

/** 
	Function used for polling the PostBoot Notification status. 
*/
TBool PollingPostBootStatus(TAny* aLddFactory)
	{
	if(aLddFactory)
		if(((DTestResManLddFactory *)aLddFactory)->iPostBootNotiCount == EXPECTED_POST_NOTI_COUNT)
			return ETrue;
	return EFalse;
	}
	
/**
  Process synchronous 'control' requests
*/
TInt DTestResManLdd::DoControl(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r = KErrNone;
	TParameterListInfo ptr = {0, 0, 0, 0, 0};
	//Copy parameter structure from user space.
	if((aFunction != RTestResMan::EDeRegisterClient) 
		&& (aFunction != RTestResMan::ERequestNotificationUncond) 
		&& (aFunction != RTestResMan::EGetIdleResourcesInfo) 
		&& (aFunction != RTestResMan::EDeRegisterClientLevelFromResource) 
		&& (aFunction != RTestResMan::ECheckPostBootLevelNotifications)
		&& (aFunction != RTestResMan::EGetControllerVersion)
		&& (aFunction != RTestResMan::ERegisterResourceController)
#ifdef PRM_ENABLE_EXTENDED_VERSION
		   && (aFunction != RTestResMan::ERegisterDynamicResource))
#else
		   )
#endif
		{
		r = Kern::ThreadRawRead(iClientThreadPtr, (TParameterListInfo*)a1, &ptr, sizeof(TParameterListInfo));
		if(r != KErrNone)
			return r;
		}
	switch(aFunction)
		{
		case RTestResMan::ERegisterClient:
			{
			TBuf8<256> aName;
			TUint c;
			//Copy name from user address space
			r = Kern::ThreadDesRead(iClientThreadPtr, (const TDesC*)ptr.iPtr2, (TDes8&)aName, 0);
			if(r != KErrNone)
				{
				Kern::Printf("RTestResMan::ERegisterClient ThreadDesRead failed with %d", r);
				break;
				}
			for(c = 0; c < MAX_CLIENTS; c++)
				{
				if(clientInfo[c].pName == NULL)
					break;
				if(!clientInfo[c].pName->Compare(aName))
					return KErrAlreadyExists;
				}
			if(c == MAX_CLIENTS)
				return KErrOverflow;
			clientInfo[c].pName = HBuf::New((const TDesC8&)aName);
			if(!clientInfo[c].pName)
				return KErrNoMemory;
			r = PowerResourceManager::RegisterClient(clientInfo[c].iClientId, (const TDesC&)*clientInfo[c].pName, (TOwnerType)(TInt)ptr.iPtr3);
			if(r != KErrNone)
				{
				delete clientInfo[c].pName;
				clientInfo[c].pName = NULL;
				clientInfo[c].iClientId = 0;
				}
			if(r == KErrNone)
				{
				r = Kern::ThreadRawWrite(iClientThreadPtr, ptr.iPtr1, &clientInfo[c].iClientId, sizeof(TUint));
				if(r != KErrNone)
					Kern::Printf("RTestResMan::ERegisterClient ThreadRawWrite failed with %d", r);
				}
			break;
			}
		case RTestResMan::EDeRegisterClient: //Deregister client from RM
			{
			//Cancel all notification pending for this client
			SNotificationList *pL;
			SNotificationList* pN = iCondList;
			while(pN != NULL)
				{
				if(pN->iClientId == (TUint)a1)
					{
					PowerResourceManager::CancelNotification(pN->iClientId, pN->iResourceId, *pN->iNoti);
					pL = pN;
					pN = pN->iNext;
					LIST_REMOVE(iCondList, pL, iNext, SNotificationList);
					delete pL->iNoti;
					delete pL;
					}
				else
					pN = pN->iNext;
				}
			pN = iUnCondList;
			while(pN != NULL)
				{
				if(pN->iClientId == (TUint)a1)
					{
					PowerResourceManager::CancelNotification(pN->iClientId, pN->iResourceId, *pN->iNoti);
					pL = pN;
					pN = pN->iNext;
					LIST_REMOVE(iUnCondList, pL, iNext, SNotificationList);
					delete pL->iNoti;
					delete pL;
					}
				else
				   pN = pN->iNext;
				}
			iClientId = -1;
			r = PowerResourceManager::DeRegisterClient((TUint)a1);
			if(r != KErrNone)
				break;
			r = KErrNotFound;
			for(TUint c = 0; c < MAX_CLIENTS; c++)
				{
				if(clientInfo[c].iClientId == (TUint)a1)
					{
					delete clientInfo[c].pName;
					clientInfo[c].pName = 0;
					clientInfo[c].iClientId = 0;
					r = KErrNone;
					break;
					}
				}

			break;
			}
		case RTestResMan::EGetClientName: //Get Client Name
			{
			TBuf8<32> aName; 
			r = PowerResourceManager::GetClientName((TUint)ptr.iClientId, (TUint)ptr.iPtr1, (TDes &)aName);
			if(r== KErrNone)
				{
				r = Kern::ThreadDesWrite(iClientThreadPtr, ptr.iPtr2, (const TDesC8&)aName, 0);
				if(r != KErrNone)
					Kern::Printf("RTestResMan::EGetClientName ThreadDesWrite failed with %d", r);
				}
			break;
			}
		case RTestResMan::EGetClientId: //Get Client Id
			{
			TBuf8<256> aName;
			TUint aClientId; 
			r = Kern::ThreadDesRead(iClientThreadPtr, (const TDesC*)ptr.iPtr1, (TDes8&)aName, 0,KChunkShiftBy0);
			if(r != KErrNone)
				{
				Kern::Printf("RTestResMan::EGetClientId ThreadDesRead failed with %d", r);
				break;
				}
			r = PowerResourceManager::GetClientId(ptr.iClientId, (TDesC&)aName, aClientId);
			if(r == KErrNone)
				{
				r = Kern::ThreadRawWrite(iClientThreadPtr, ptr.iPtr2, &aClientId, sizeof(TUint));
				if(r != KErrNone)
					Kern::Printf("RTestResMan::EGetClientId ThreadRawWrite failed with %d", r);
				}
			break;
			}
		case RTestResMan::EGetResourceId: //Get Resource Id
			{
			TBuf8<256> aName; 
			TUint aResourceId;
			r = Kern::ThreadDesRead(iClientThreadPtr, (const TDesC*)ptr.iPtr1, (TDes8&)aName, 0,KChunkShiftBy0);
			if(r != KErrNone)
				{
				Kern::Printf("RTestResMan::EGetResourceId ThreadDesRead failed with %d", r);
				break;
				}
			r = PowerResourceManager::GetResourceId(ptr.iClientId, (TDesC&)aName, aResourceId);
			if(r == KErrNone)
				{
				r = Kern::ThreadRawWrite(iClientThreadPtr, ptr.iPtr2, &aResourceId, sizeof(TUint));
				if(r != KErrNone)
					Kern::Printf("RTestResMan::EGetResourceId ThreadRawWrite failed with %d", r);
				}
			break;
			}
		case RTestResMan::EGetResourceInfo: //Get resource information
			{
			NKern::ThreadEnterCS();
			HBuf *info = HBuf::New(sizeof(TPowerResourceInfoV01));
			NKern::ThreadLeaveCS();
			r = PowerResourceManager::GetResourceInfo(ptr.iClientId, (TUint)ptr.iPtr1, (TAny*)info);
			if(r == KErrNone)
				{
				r = Kern::ThreadDesWrite(iClientThreadPtr, ptr.iPtr2, (const TDesC8&)*info, 0);
				if(r != KErrNone)
					Kern::Printf("RTestResMan::EGetResourceInfo ThreadRawWrite failed with %d", r);
				}
			Kern::Free(info);
			break;
			} 
		case RTestResMan::EGetNumResourcesInUseByClient: 
			{
			TUint numResource;
			r = PowerResourceManager::GetNumResourcesInUseByClient(ptr.iClientId, (TUint)ptr.iPtr1, numResource);
			if(r == KErrNone)
				{
				r = Kern::ThreadRawWrite(iClientThreadPtr, ptr.iPtr2, &numResource, sizeof(TUint));
				if(r != KErrNone)
					Kern::Printf("RTestResMan::EGetNumResourcesInUseByClient ThreadRawWrite failed with %d", r);
				}
			 break;
			}
		case RTestResMan::EGetInfoOnResourcesInUseByClient:
			{
			TUint numResource;
			HBuf* info = NULL;
			r = Kern::ThreadRawRead(iClientThreadPtr, ptr.iPtr2, &numResource, sizeof(TUint));
			if(r != KErrNone)
				{
				Kern::Printf("RTestResMan::GetInfoOnResourceInUseByClient ThreadRawRead failed with %d", r);
				break;
				}
			if(ptr.iPtr3 != NULL)
				{
				NKern::ThreadEnterCS();
				info = HBuf::New(numResource * sizeof(TPowerResourceInfoV01));
				NKern::ThreadLeaveCS();
				}
			r = PowerResourceManager::GetInfoOnResourcesInUseByClient(ptr.iClientId, (TUint)ptr.iPtr1, numResource, (TAny*)info);
			if(r == KErrNone)
				{
				r = Kern::ThreadRawWrite(iClientThreadPtr, ptr.iPtr2, &numResource, sizeof(TUint));
				if(r !=KErrNone)
					{
					Kern::Printf("RTestResMan::GetInfoOnResourceInUseByClient ThreadRawWrite failed with %d", r);
					Kern::Free(info);
					break;
					}
				if(ptr.iPtr3)
					r = Kern::ThreadDesWrite(iClientThreadPtr, ptr.iPtr3, (const TDesC8&)*info, 0);
				}
			Kern::Free(info);
			break;
			}
		case RTestResMan::EGetNumClientsUsingResource:
			{
			TUint numClient;
			r = PowerResourceManager::GetNumClientsUsingResource(ptr.iClientId, (TUint)ptr.iPtr1, numClient);
			if(r == KErrNone)
				{
				r = Kern::ThreadRawWrite(iClientThreadPtr, ptr.iPtr2, &numClient, sizeof(TUint));
				if(r != KErrNone)
					Kern::Printf("RTestResMan::EGetNumResourcesInUseByClient ThreadRawWrite failed with %d", r);
				}
			break;
			}
		case RTestResMan::EGetInfoOnClientsUsingResource:
			{
			TUint numClient;
			HBuf* info = NULL;
			r = Kern::ThreadRawRead(iClientThreadPtr, ptr.iPtr2, &numClient, sizeof(TUint));
			if(r != KErrNone)
				{
				Kern::Printf("RTestResMan::GetInfoOnResourceInUseByClient ThreadRawRead failed with %d", r);
				break;
				}
			if(ptr.iPtr3 != NULL)
				{
				NKern::ThreadEnterCS();
				info = HBuf::New(numClient * sizeof(TPowerClientInfoV01));
				NKern::ThreadLeaveCS();
				}
			r = PowerResourceManager::GetInfoOnClientsUsingResource(ptr.iClientId, (TUint)ptr.iPtr1, numClient, (TAny*)info);
			if(r == KErrNone)
				{
				r = Kern::ThreadRawWrite(iClientThreadPtr, ptr.iPtr2, &numClient, sizeof(TUint));
				if(r !=KErrNone)
					{
					Kern::Printf("RTestResMan::GetInfoOnResourceInUseByClient ThreadRawWrite failed with %d", r);
					Kern::Free(info);
					break;
					}
				 if(ptr.iPtr3)
					r = Kern::ThreadDesWrite(iClientThreadPtr, ptr.iPtr3, (const TDesC8&)*info, 0);
				}
			Kern::Free(info);
			break;
			}
		case RTestResMan::EAllocReserve:
			{
			r = PowerResourceManager::AllocReserve(ptr.iClientId, (TUint8)(TUint)ptr.iPtr1, (TUint8)(TUint)ptr.iPtr2);
			break;
			}
		case RTestResMan::EChangeResourceStateSync:
			{
			SNotificationList* pN; 
			//Zero the conditional notification count for the resource id.
			for(pN=iCondList; pN != NULL; pN=pN->iNext)
				{
				if(pN->iResourceId == (TUint)ptr.iPtr1)
					{
					pN->iCount = 0;
					break;
					}
				}
			//Zero the unconditional notification count for the resource id.
			for(pN=iUnCondList; pN != NULL; pN=pN->iNext)
				{
				if(pN->iResourceId == (TUint)ptr.iPtr1)
					{
					pN->iCount = 0;
					break;
					}
				}
			iClientId = ptr.iClientId;
			iResourceId = (TUint)ptr.iPtr1;
			r = PowerResourceManager::ChangeResourceState(ptr.iClientId, (TUint)ptr.iPtr1, (TInt)ptr.iPtr2);
			break;
			}
		case RTestResMan::EGetResourceStateSync:
			{
			TInt newState, levelOwnerId;
			r = PowerResourceManager::GetResourceState(ptr.iClientId, (TUint)ptr.iPtr1, (TBool)ptr.iPtr2, newState, levelOwnerId);
			if(r == KErrNone)
				{
				r = Kern::ThreadRawWrite(iClientThreadPtr, ptr.iPtr3, (TAny*)&newState, sizeof(TInt));
				if(r != KErrNone)
					{
					Kern::Printf("RTestResMan::GetResourceStateSync ThreadRawWrite failed with %d", r);
					break;
					}
				r = Kern::ThreadRawWrite(iClientThreadPtr, ptr.iPtr4, (TAny*)&levelOwnerId, sizeof(TInt));
				if(r != KErrNone)
					Kern::Printf("RTestResMan::GetResourceStateSync ThreadRawWrite failed with %d", r);
				}
			break;
			}
		case RTestResMan::ERequestNotificationUncond:
			{
			SNotificationList *notiList;
			notiList = new SNotificationList; //Create new notification list
			if(!notiList)
				{
				r = KErrNoMemory;
				break;
				}
			//Create notification object to pass to RM
			notiList->iNoti = new DPowerResourceNotification(UnCondNotificationFunc, this, ((DTestResManLddFactory*)iDevice)->iDfcQ, 3);
			if(!notiList->iNoti)
				{
				delete notiList;
				r = KErrNoMemory;
				break;
				}
			notiList->iClientId = (TUint)a1;
			notiList->iResourceId = (TUint)a2;
			notiList->iCount = 0;
			LIST_PUSH(iUnCondList, notiList, iNext); //Add to unconditional list.							 
			r = PowerResourceManager::RequestNotification((TUint)a1, (TUint)a2, *notiList->iNoti);
			break;
			}
		case RTestResMan::ERequestNotificationCond:
			{
			SNotificationList *notiList;
			notiList = new SNotificationList; // Create new notification list
			if(!notiList)
				{
				r = KErrNoMemory;
				break;
				}
			//Create notification object to pass to RM
			notiList->iNoti = new DPowerResourceNotification(CondNotificationFunc, this, ((DTestResManLddFactory*)iDevice)->iDfcQ, 3);
			if(!notiList->iNoti)
				{
				delete notiList;
				r = KErrNoMemory;
				break;
				}
			notiList->iClientId = ptr.iClientId;
			notiList->iResourceId = (TUint)ptr.iPtr1;
			notiList->iCount = 0;
			LIST_PUSH(iCondList, notiList, iNext); //Add to conditional list.
			r = PowerResourceManager::RequestNotification((TUint)ptr.iClientId, (TUint)ptr.iPtr1, *notiList->iNoti, (TInt)ptr.iPtr2, (TBool)ptr.iPtr3);
			break;
			}
		case RTestResMan::EDeRegisterClientLevelFromResource:	
			{
				r = PowerResourceManager::DeRegisterClientLevelFromResource((TUint)a1, (TUint)a2);
				break;
			} 
		case RTestResMan::ECancelNotification:
			{
			r = KErrNotFound;
			if(ptr.iPtr2) //Check for notification in conditional list if it is true.
				{ 
				for(SNotificationList* pN=iCondList; pN != NULL; pN=pN->iNext)
					{
					if((pN->iClientId == ptr.iClientId) && (pN->iResourceId == (TUint)ptr.iPtr1))
						{
						r = PowerResourceManager::CancelNotification(pN->iClientId, pN->iResourceId, *pN->iNoti);
						LIST_REMOVE(iCondList, pN, iNext, SNotificationList);
						delete pN->iNoti;
						delete pN;
						break;
						}
					}
				}
			else //Check for notification in unconditional list.
				{
				for(SNotificationList* pN=iUnCondList; pN != NULL; pN=pN->iNext)
					{
					if((pN->iClientId == ptr.iClientId) && (pN->iResourceId == (TUint)ptr.iPtr1))
						{
						r = PowerResourceManager::CancelNotification(pN->iClientId, pN->iResourceId, *pN->iNoti);
						LIST_REMOVE(iUnCondList, pN, iNext, SNotificationList);
						delete pN->iNoti;
						delete pN;
						break;
						}
					}
				}
			break;
			}
		case RTestResMan::ECheckNotifications:
			{
			NKern::Sleep(NKern::TimerTicks(300)); //This is required as sometimes check is done before callback is called.
			TUint countCond = 0, countUncond = 0;
			SNotificationList* pN; 
			//Get the conditional notification callback functions called for the resource id.
			for(pN=iCondList; pN != NULL; pN=pN->iNext)
				{
				if(pN->iResourceId == (TUint)ptr.iPtr1)
					{
					countCond = pN->iCount;
					pN->iCount = 0;
					break;
					}
				}
			//Get the unconditional notification callback functions called for the resource id.
			for(pN=iUnCondList; pN != NULL; pN=pN->iNext)
				{
				if(pN->iResourceId == (TUint)ptr.iPtr1)
					{
					countUncond = pN->iCount;
					pN->iCount = 0;
					break;
					}
				}
			//If the notifications count is not as expected return error.
			if((countCond != (TUint)ptr.iPtr3) || (countUncond != (TUint)ptr.iPtr2))
				r = KErrUnderflow;
			break;
			}
		case RTestResMan::ERegisterForIdleResourcesInfo:
			{	
			if(pBuf)
				{
				r = KErrAlreadyExists;
				break;
				}
			NKern::ThreadEnterCS();
			pBuf = HBuf::New((TUint)ptr.iPtr1 * sizeof(SIdleResourceInfo)); //Allocate buffer for requested resources
			NKern::ThreadLeaveCS();
			if(!pBuf)
				return KErrNoMemory;
			r = Kern::ThreadRawRead(iClientThreadPtr, ptr.iPtr2, (TAny*)pBuf->Ptr(), (TUint)ptr.iPtr1 * sizeof(SIdleResourceInfo));
			if(r != KErrNone)
				{
				Kern::Printf("RTestResMan::ERegisterForIdleResourceInfo threadRawRead failed with %d\n", r);
				break;
				}
			//Below function calls RegisterForResourceIdle resource controller virtual function, 
			//This is for testing purposes only.

			r =DSimulatedPowerResourceController::CaptureIdleResourcesInfo((TUint)ptr.iClientId, (TUint)ptr.iPtr1, (TPtr*)pBuf);
			if( r == KErrInUse)
			   delete pBuf;
			break;
			}
		case RTestResMan::EGetIdleResourcesInfo:
			{
			//Pass the buffer for comparision
			if(!pBuf)
				{
				r = KErrNotFound;
				break;
				}
			pBuf->SetLength(sizeof(SIdleResourceInfo) * (TUint)a1);
			r = Kern::ThreadDesWrite(iClientThreadPtr, a2, (const TDesC8&)*pBuf, 0);
			break;
			}
		case RTestResMan::ECheckPostBootLevelNotifications:
			{
			//aPollPeriodMs = 3ms (3rd argument)
			//aMaxPoll = 1000 (in total ~3000 ms timeout)
			r = Kern::PollingWait(PollingPostBootStatus, (TAny*)iDevice, 3, 1000);
			break;
			}
		case RTestResMan::EGetControllerVersion:
			{
			TUint Version;
			r = PowerResourceManager::GetResourceControllerVersion((TUint)a1, Version);
			if(r == KErrNone)
				{
				r = Kern::ThreadRawWrite(iClientThreadPtr, a2, &Version, sizeof(TUint));
				if(r != KErrNone)
					Kern::Printf("RTestResMan::EGetControllerVersion ThreadRawWrite failed with %d", r);
				}
			break;
			}
		case RTestResMan::ERegisterResourceController:
			{
			r = DSimulatedPowerResourceController::ResourceControllerRegistration();
			break;
			}
#ifdef PRM_ENABLE_EXTENDED_VERSION
		case RTestResMan::ERegisterDynamicResource:
			{
			TUint resId;
			r = Kern::ThreadRawRead(iClientThreadPtr, a2, (TAny*)&resId, sizeof(TUint));
			if(r != KErrNone)
				{
				Kern::Printf("RTestResMan::RegisterDynamicResource ThreadRawRead failed with %d", r);
				break;
				}			
			DDynamicPowerResource* pDR = NULL;
			switch(resId) //Create the dynamic resource
				{
				case 1:
					pDR = new (DBIGISSPDynamicResource);
					break;
				case 2:
					pDR = new (DMLIGLSSHNDynamicResource);
					break;
				case 3: 
					pDR = new (DBLGLSSHNDynamicResource);
					break;
				case 4:
					pDR = new (DMLLGLSSHPDynamicResource);
					break;
				case 5:
					pDR = (DDynamicPowerResource*) new (DDynamicResourceD01);
					break;
				case 6:
					pDR = (DDynamicPowerResource*) new (DDynamicResourceD02);
					break;
				case 7:
					pDR = (DDynamicPowerResource*) new (DDynamicResourceD03);
					break;
				case 8:
					pDR = (DDynamicPowerResource*) new (DDynamicResourceD04);
					break;
				}
			if(!pDR)
				return KErrNoMemory;
			iDynamicArray.Append(pDR);
			r = PowerResourceManager::RegisterDynamicResource((TUint)a1, pDR, resId);
			if(r == KErrNone)
				{
				r = Kern::ThreadRawWrite(iClientThreadPtr, a2, (TAny*)&resId, sizeof(TUint));
				if(r != KErrNone)
					Kern::Printf("RTestResMan::RegisterDynamicResource ThreadRawWrite failed with %d", r);
				}
			break;
			}
		case RTestResMan::EDeRegisterDynamicResource: //Deregister dynamic resource
			{
			if(ptr.iPtr2)
				{
				TInt level;
				r = Kern::ThreadRawRead(iClientThreadPtr, ptr.iPtr2, &level, sizeof(TInt));
				if(r != KErrNone)
					{
					Kern::Printf("RTestResMan::DeRegisterDynamicResource ThreadRawRead failed with %d", r);
					break;
					}	
				r = PowerResourceManager::DeRegisterDynamicResource(ptr.iClientId, (TUint)ptr.iPtr1, &level);
				}
			else
				r = PowerResourceManager::DeRegisterDynamicResource(ptr.iClientId, (TUint)ptr.iPtr1, NULL);
			break;
			}
		case RTestResMan::ERegisterResourceDependency: //Register resource dependency
			{
			SResourceDependencyInfo info1, info2;
			r = Kern::ThreadRawRead(iClientThreadPtr, ptr.iPtr1, &info1, sizeof(SResourceDependencyInfo));
			if(r != KErrNone)
				{
				Kern::Printf("RTestResMan::RegisterResourceDependency ThreadRawRead failed with %d", r);
				break;
				}
			r = Kern::ThreadRawRead(iClientThreadPtr, ptr.iPtr2, &info2, sizeof(SResourceDependencyInfo));
			if(r != KErrNone)
				{
				Kern::Printf("RTestResMan::RegisterResourceDependency ThreadRawRead failed with %d", r);
				break;
				}
			r = PowerResourceManager::RegisterResourceDependency(ptr.iClientId, &info1, &info2);
			break;
			}
		case RTestResMan::EDeRegisterResourceDependency: //Deregister resource dependency
			{
			r = PowerResourceManager::DeRegisterResourceDependency(ptr.iClientId, (TUint)ptr.iPtr1, (TUint)ptr.iPtr2);
			break;
			}
		case RTestResMan::EGetNumDependentsForResource:
			{
			TUint numDepResources;
			r = PowerResourceManager::GetNumDependentsForResource(ptr.iClientId, (TUint)ptr.iPtr1, numDepResources);
			if(r == KErrNone)
				{
				r = Kern::ThreadRawWrite(iClientThreadPtr, ptr.iPtr2, (TAny*)&numDepResources, sizeof(TUint));
				if(r != KErrNone)
					Kern::Printf("RTestResMan::RegisterDynamicResource ThreadRawWrite failed with %d", r);
				}
			break;
			}
		case RTestResMan::EGetDependentsIdForResource:
			{
			TUint numDepResources;
			HBuf* sResDepInfo = NULL;
			r = Kern::ThreadRawRead(iClientThreadPtr, ptr.iPtr3, (TAny*)&numDepResources, sizeof(TUint));
			if(r != KErrNone)
				{
				Kern::Printf("RTestResMan::GetDependentsIdForResource ThreadRawRead failed with %d", r);
				break;
				}
			if(ptr.iPtr2 != NULL)
				{
				NKern::ThreadEnterCS();
				sResDepInfo = HBuf::New(numDepResources * sizeof(SResourceDependencyInfo));
				NKern::ThreadLeaveCS();
				if(!sResDepInfo)
					return KErrNoMemory;
				}
			
			r = PowerResourceManager::GetDependentsIdForResource(ptr.iClientId, (TUint)ptr.iPtr1, (TAny*)sResDepInfo, numDepResources);
			if(r == KErrNone)
				{
				if(ptr.iPtr2)
					r = Kern::ThreadDesWrite(iClientThreadPtr, ptr.iPtr2, (const TDesC8&)*sResDepInfo, 0);
				if(r != KErrNone)
					{
					Kern::Printf("RTestResMan::GetDepedentsIdForResource ThreadDesWrite failed with %d", r);
					Kern::Free(sResDepInfo);
					break;
					}
				r = Kern::ThreadRawWrite(iClientThreadPtr, ptr.iPtr3, (TAny*)&numDepResources, sizeof(TUint));
				if(r != KErrNone)
					{
					Kern::Printf("RTestResMan::GetDependentsIdForResource ThreadRawWrite failed with %d", r);
					Kern::Free(sResDepInfo);
					break;
					}
				}
			Kern::Free(sResDepInfo);
			break;
			}

#endif
		default:
			r = KErrNotSupported;
			break;
		}
	return r;
	}

TInt DTestResManLdd::DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* /*a2*/)
	{
	TInt r = KErrNone;
	TParameterListInfo ptr;
	r = Kern::ThreadRawRead(iClientThreadPtr, (TParameterListInfo*)a1, &ptr, sizeof(TParameterListInfo));
	if(r != KErrNone)
		Kern::RequestComplete(iClientThreadPtr, aStatus, r);
	switch(aReqNo)
		{
		case RTestResMan::EChangeResourceStateAsync:
#ifdef PRM_ENABLE_EXTENDED_VERSION
		case RTestResMan::EChangeResStateAndDeregisterDynamicRes:
		case RTestResMan::ECheckParallelExecutionForChangeResState:
#endif
			SNotificationList* pN; 
			//Zero the conditional notification count for the resource id.
			for(pN=iCondList; pN != NULL; pN=pN->iNext)
				{
				if(pN->iResourceId == (TUint)ptr.iPtr1)
					{
					pN->iCount = 0;
					break;
					}
				}
			//Zero the unconditional notification count for the resource id.
			for(pN=iUnCondList; pN != NULL; pN=pN->iNext)
				{
				if(pN->iResourceId == (TUint)ptr.iPtr1)
					{
					pN->iCount = 0;
					break;
					}
				}
			iLevelOwnerIdPtr = NULL;
			iStatePtr = (TInt*)ptr.iPtr2;
			TInt state;
			r = Kern::ThreadRawRead(iClientThreadPtr, iStatePtr, &state, sizeof(TInt)); 
			if(r != KErrNone)
				Kern::RequestComplete(iClientThreadPtr, aStatus, r);
			iStatus = aStatus;
			iClientId = ptr.iClientId;
			iResourceId = (TUint)ptr.iPtr1;
			iCallbackCancel = EFalse;
			r = PowerResourceManager::ChangeResourceState(ptr.iClientId, (TUint)ptr.iPtr1, state, &iAsyncResourceCallback);
#ifdef PRM_ENABLE_EXTENDED_VERSION
			if(aReqNo == RTestResMan::EChangeResStateAndDeregisterDynamicRes) //Try to delete the dynamic while resource change
				{
				r = PowerResourceManager::DeRegisterDynamicResource(ptr.iClientId, (TUint)ptr.iPtr1, NULL);
				if(r == KErrInUse) //Wait for the request to complete
					r = KErrNone; 
				break;
				}
			if(aReqNo == RTestResMan::ECheckParallelExecutionForChangeResState)
				{
				r = PowerResourceManager::ChangeResourceState(ptr.iClientId, (TUint)ptr.iPtr3, (TInt)ptr.iPtr4, &iAsyncTestParallelCallback);
				iTestParallelResourceId = (TUint)ptr.iPtr3;
				iValidateCallbackReceived = ETrue;
				break;
				}
#endif
			if(ptr.iPtr3) //Cancel the asynchronous operation if true.
				{
				r = PowerResourceManager::CancelAsyncRequestCallBack(ptr.iClientId, (TUint)ptr.iPtr1, iAsyncResourceCallback);
				if(r == KErrInUse) //Wait for the request to complete
					r = KErrNone;
				else
					iCallbackCancel = ETrue;
				}
			break;
		case RTestResMan::EGetResourceStateAsync:
			iStatus = aStatus;
			iCallbackCancel = EFalse;
			iClientId = ptr.iClientId;
			iResourceId = (TUint)ptr.iPtr1;
			iStatePtr = (TInt*)ptr.iPtr4;
			iLevelOwnerIdPtr = (TInt*)ptr.iPtr5;
			r = PowerResourceManager::GetResourceState(ptr.iClientId, (TUint)ptr.iPtr1, (TBool)ptr.iPtr2, iAsyncResourceCallback);
			if(ptr.iPtr3) //Cancel the asynchronous operation if true.
				{
				r = PowerResourceManager::CancelAsyncRequestCallBack(ptr.iClientId, (TUint)ptr.iPtr1, iAsyncResourceCallback);
				if(r == KErrInUse)
					r = KErrNone;
				else 
					iCallbackCancel = ETrue;
				}
			break;
		}
	return r;
	}

//Function called on Asynchronous operation
void DTestResManLdd::CallbackFunc(TUint aClientId, TUint aResId, TInt aLevel, TInt aLevelOwnerId, TInt aResult, TAny* aParam)
	{
	TInt r;
	DTestResManLdd *pC = (DTestResManLdd*)aParam;
	//Check for correctnes of clientId and resourceId
	if((TUint)pC->iClientId != aClientId || pC->iResourceId != aResId)
		Kern::RequestComplete(pC->iClientThreadPtr, pC->iStatus, KErrCorrupt);
	if(!pC->iCallbackCancel)
		{
		if(pC->iStatePtr) 
			{
			r = Kern::ThreadRawWrite(pC->iClientThreadPtr, pC->iStatePtr, (TAny*)&aLevel, sizeof(TInt));
			if(r != KErrNone)
				Kern::Printf("RTestResManLdd::CallbackFunc ThreadRawWrite failed with %d", r);
			}
		if(pC->iLevelOwnerIdPtr)
			{
			r = Kern::ThreadRawWrite(pC->iClientThreadPtr, pC->iLevelOwnerIdPtr, (TAny*)&aLevelOwnerId, sizeof(TInt));
			if(r != KErrNone)
				Kern::Printf("RTestResManLdd::CallbackFunc ThreadRawWrite failed with %d", r);
			}
		#ifdef PRM_ENABLE_EXTENDED_VERSION
		if(pC->iValidateCallbackReceived)
			{
			if(!pC->iCallbackReceived)
				aResult = KErrCompletion;
			}
		#endif
		Kern::RequestComplete(pC->iClientThreadPtr, pC->iStatus, aResult);
		}
	pC->iCallbackCancel = EFalse;
	pC->iStatus = NULL;
	}

#ifdef PRM_ENABLE_EXTENDED_VERSION
//Function called on completion of asynchronous long latency resource, used only to check parallel execution of DFC's
void DTestResManLdd::TestParallelExecutionCallback(TUint aClientId, TUint aResId, TInt /*aLevel*/, TInt /*aLevelOwnerId*/, TInt /*aResult*/, TAny* aParam)
	{
	Kern::Printf("DTestResManLdd::TestParallelExecutionCallback:: called");
	DTestResManLdd *pC = (DTestResManLdd*)aParam;
	//Check for correctness of clientId and resourceId
	if((TUint)pC->iClientId == aClientId && pC->iTestParallelResourceId == aResId)
		{
		pC->iCallbackReceived = ETrue;
		}
	}
#endif

//Function called on Conditional notification
void DTestResManLdd::CondNotificationFunc(TUint /*aClientId*/, TUint aResId, TInt /*aLevel*/, TInt /*aLevelOwnerId*/, TInt /*aResult*/, TAny* aParam)
	{
	DTestResManLdd *pC = (DTestResManLdd*)aParam;
	for(SNotificationList *pN = pC->iCondList; pN!= NULL; pN=pN->iNext)
		{
		if((pN->iResourceId == aResId))
			{
			pN->iCount++; //Increment the count, as same callback function for all conditioanl notifications.
			break;
			}
		}
	}

//Function called on UnConditional notification
void DTestResManLdd::UnCondNotificationFunc(TUint /*aClientId*/, TUint aResId, TInt /*aLevel*/, TInt /*aLevelOwnerId*/, TInt /*aResult*/, TAny* aParam)
	{
	DTestResManLdd *pC = (DTestResManLdd*)aParam;
	for(SNotificationList *pN = pC->iUnCondList; pN!= NULL; pN=pN->iNext)
		{
		if((pN->iResourceId == aResId) && (pC->iCallbackCancel == EFalse))
			{
			pN->iCount++; //Increment the count as same callback function for all unconditioanl notifications.
			break;
			}
		}
	}

//Function called on postbootvalueset. 
void DTestResManLddFactory::PostBootNotificationFunc(TUint /*aClientId*/, TUint aResId, TInt /*aLevel*/, TInt /*aLevelOwnerId*/, TInt /*aResult*/, TAny* aParam)
	{
	iPostBootNotiCount++;
	DPowerResourceNotification *ptr = (DPowerResourceNotification*)aParam;
	TInt r = PowerResourceManager::CancelNotification(iClient.iClientId, aResId, *ptr);
	if(r == KErrCancel)
		{
		ptr->AsyncDelete();
		if(iPostBootNotiCount == EXPECTED_POST_NOTI_COUNT)
			{
			r = PowerResourceManager::DeRegisterClient(DTestResManLddFactory::iClient.iClientId);
			if(r != KErrNone)
				Kern::Fault("PRM CLIENT DEREGISTER FAILED", __LINE__);
			delete DTestResManLddFactory::iClient.pName;
			DTestResManLddFactory::iClient.pName = NULL;
			DTestResManLddFactory::iClient.iClientId = 0;
			}
		}
	}
