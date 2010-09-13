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
// e32test\resourceman\t_rescontrolcli.cpp
// TestCase Description:
// This tests is intended to test the generic layer of PRM. It consists of unit testing and regression testing. 
// Unit testing validates each of the API's. Regression testing performs random operation on random resource and
// currently executes 500 operations and returns. 
// To run regression testing, test must be invoked with -R option. 
// Testing runs only on simulated PSL. 
// 
//

#include <e32test.h>
#include <e32hal.h>
#include <e32math.h>
#include <e32def.h>
#include <e32def_private.h>
#include "d_rescontrolcli.h"

#include <drivers/resourcecontrol_clientsettings.h>
#include <drivers/resourcecontrol_settings.h>

#define MAX_STATIC_RESOURCE_NUM 24 //Maximum number of static resources in simulated PSL
#define MAX_STATIC_DEPENDENCY_RESOURCE_NUM 7 //Maximum number of static dependency resources in simulated PSL
#define DEPENDENCY_RESOURCE_BIT_MASK 0x00010000
#define DYNAMIC_RESOURCE_BIT_MASK 0x00020000
#define CUSTOM_RESOURCE_NUMBER 20

/** Macro to push the item into the specified list. Item are pushed to the head of the list. */
#define LIST_PUSH(list,item,link)	\
	{							   	\
	(item)->link = (list);			\
	(list) = (item);				\
	}

/** Macro to pop the item from the specified list. Item are poped from the head of the list. */
#define LIST_POP(list,item,link)	\
	{							   	\
	(item) = (list);				\
	if ((item))						\
		{							\
		(list) = (item)->link;		\
		(item)->link = NULL;		\
		}						   	\
	}

/** Macro to remove the item from the list. */
#define LIST_REMOVE(list,item,link,className)		\
	if (list)										\
		{											\
		className* current = (list);				\
		if (current==(item))						\
			{										\
			(list) = (item)->link;					\
			(item)->link = NULL;					\
			}										\
		else										\
			{										\
			className* next = current->link;		\
			while (next)							\
				{									\
				if ((item)==next)					\
					{								\
					current->link=next->link;		\
					next->link = NULL;				\
					break;							\
					}								\
				current = next;						\
				next = next->link;					\
				}									\
			}										\
		}

#ifndef PRM_ENABLE_EXTENDED_VERSION
_LIT(KLddFileName, "D_RESCONTROLCLI.LDD");
_LIT(KPddFileName, "resourcecontroller.pdd");
#else
_LIT(KExtLddFileName, "D_EXTENDEDRESCONTROLCLI.LDD");
_LIT(KExtPddFileName, "resourcecontrollerextended.pdd");
#endif

LOCAL_D RTest test(_L("RESOURCE_MANAGER_TEST"));
TBuf8<32> SpecialResName(_L8("SymbianSimulResource"));

//Enum definition for resource classification.
enum TType  {EMultiLevel = 0x1, EMultiProperty};
enum TUsage {ESingle, EShared};
enum TLatency {EInstantaneous, ELongLatency};
enum TClass {EPhysical, ELogical};
enum TSense {EPositive, ENegative, ECustom};
	
//Structure to get resource information
class TPowerResourceInfoV01
	{
public:
	TClass iClass;
	TLatency iLatencyGet;
	TLatency iLatencySet;
	TType iType;
	TUsage iUsage;
	TSense iSense;
	TDesC8* iResourceName;
	TUint iResourceId;
	TInt iDefaultLevel;  
	TInt iMinLevel;	  
	TInt iMaxLevel;	  
	TInt iReserved1;  
	TInt iReserved2;  
	TInt iReserved3;  
	TInt iPslReserved1;  
	TInt iPslReserved2;  
	TInt iPslReserved3;  
	};

//Structure to get client information
struct TPowerClientInfoV01
	{
	TUint iClientId;
	TDesC8* iClientName;
	};

//Structure for holding client information
struct RMClientInfo
	{
	TUint iClientId;
	TUint iNumResources;
	TUint iResourceIds[MAX_STATIC_RESOURCE_NUM]; //Each bit corresponds to a static resource.
	};

//Structure for holding notification information
struct NotiInfo
	{
	TUint iClientId;
	TInt iThreshold;
	TInt iDirection;
	TInt iPreviousLevel;
	NotiInfo *iNext;
	};

// Structure for holding client level
struct SPowerResourceClientLevel
	{
	TUint iClientId;
	TUint iResourceId;
	TInt iLevel;
	SPowerResourceClientLevel* iNextInList;
	};

//Structure for capturing resource information to be used by Idle thread.
struct SIdleResourceInfo
	{
	TUint iResourceId;
	TInt iLevelOwnerId;
	TInt iCurrentLevel;
	TInt iReserved1;	//Reserved for future use.
	TInt iReserved2;	//Reserved for future use.
	TInt iReserved3;	//Reserved for future use.
	};

//Structure for holding resource information
struct RMResInfo
	{
	TBuf8<32> iName;
	TUint iResourceId;
	TInt iMaxLevel;
	TInt iMinLevel;
	TInt iDefaultLevel;
	TInt iCurrentLevel;
	TInt iCurrentClient;
	TUint iNumClients;
	TSense iSense;
	TType iType;
	TLatency iLatencyGet;
	TLatency iLatencySet;
	TUsage iUsage;
	TUint iUnCondNotiCount;
	NotiInfo *iCondNoti;
	NotiInfo *iUncondNoti;
	SPowerResourceClientLevel *iLevel;
	};

//Test class.
class TestRM
	{
private:
	//Possible resource operation.
	enum Operation
		{
		ERegisterClient = 0,
		EGetClientName = 1,
		EGetAllClientName = 2,
		EGetClientId = 3,
		EGetResourceId = 4,
		EGetResourceInfo = 5,
		EGetNumReosourceInUseByClient = 6,
		EGetInfoOnResourceInUseByClient = 7,
		EGetNumClientsUsingResource = 8,
		EGetInfoOnClientsUsingResource = 9,
		EChangeResourceStateSync = 10,
		EChangeResourceStateAsync = 11,
		EGetResourceStateSync = 12,
		EGetResourceStateAsync = 13,
		ERequestNotificationCond = 14,
		ERequestNotificationUnCond = 15,
		ECancelNotificationCond = 16,
		ECancelNotificationUnCond = 17,
		EOperationEnd = 18
		};
public:
	TestRM();
	void RegisterClient();
	void DeRegisterClient(TUint aClientId);
	void GetClientName(TUint aClientId);
	void GetClientId(TUint aClientId);
	void GetResourceId(TUint aResId);
	void GetResourceInfo(TUint aResId);
	void GetNumResourcesInUseByClient(TInt aClientId);
	void GetInfoOnResourcesInUseByClient(TInt aClientId, TUint aNumRes);
	void GetNumClientsUsingResource(TUint aClientId, TUint aResId);
	void GetInfoOnClientsUsingResource(TUint aResId, TUint aNumCli);
	void AllocReserve(TUint aClientId);
	void CheckNotification(TUint aResId, TInt newState);
	void AddClientLevel(TUint aResId, TInt newState);
	void UpdateClientInformation(TUint aResId, TInt aNewState);
	void ChangeResourceStateSync(TUint aResId);
	void ChangeResourceStateAsync(TUint aResId, TBool aReqCancel=EFalse);
	void GetResourceStateSync(TUint aResId);
	void GetResourceStateAsync(TUint aResId, TBool aReqCancel=EFalse);
	void RequestNotification(TUint aResId);
	void RequestNotificationCon(TUint aResId);
	void ValidateClient(TUint aNumClients, TOwnerType aContext);
	void CancelNotification(TUint aResId, TBool Cond);
	void APIValidationTest();
	void SharedBinaryPositiveResourceTesting(TUint aResId);
	void SharedBinaryNegativeResourceTesting(TUint aResId);
	void CustomResourceTesting(TUint aResId);
	void DeRegisterClientLevelFromResource(TInt aClientId, TUint aResId);
	void RegressionTest();
	void TestStaticResourceWithDependency();
	void GetExtendedResStateAsyncAndVerify(TUint aResId, TInt aState, TInt aLevelOwnerId, TBool aReqCancel = EFalse);
	void GetExtendedResStateAndVerify(TUint aResId, TInt aState, TInt aLevelOwnerId);
	void TestDynamicResource();
	void TestDynamicResourceDependency();
	void CheckForDependencyInformation(TUint aClientId, TUint aResourceId, TUint aNumDependents, SResourceDependencyInfo* aDepResIdArray);
	void SharedMultilevelNegativeResourceTesting(TUint aResId);
	void SharedMultilevelPositiveResourceTesting(TUint aResId);
private:
	RArray<RMClientInfo> Clients;
	RMResInfo Resources[MAX_STATIC_RESOURCE_NUM];
	TUint iStaticDependencyResources[MAX_STATIC_DEPENDENCY_RESOURCE_NUM];
	TInt iCurrentClientId;
	TUint iMaxClientId;
	TUint iMaxClients;
	TUint iMaxStaticResources;
	TUint iMaxStaticDependentResources;
	TUint iPowerControllerId;
	TUint iTestingExtendedVersion;
	};

TBool NegativeTesting; //If true enables negative testing of API's
TInt r = KErrNone;
TBuf8<32> ClientName(_L8("Client?"));
RTestResMan lddChan;
TestRM RmTest;

//Class constructor
TestRM::TestRM(): iCurrentClientId(-1),iMaxClientId(0), iMaxClients(0),iMaxStaticResources(0), iMaxStaticDependentResources(0), iTestingExtendedVersion(0)
	{
	test.Printf(_L("TestRM::TestRM()\n"));
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0573
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests the client registeration functionality of resource
//!				manager.It registeres a client with resource manager and stores the relevant 
//!				information in Clients array. Currently allows only maximum of 50 client 
//!				registration.
//! @SYMTestActions	 0	Returns if already maximum allowable clients are registered. 
//!				1	Register a client with the resource manager with a unique name.
//!				2	Appends the client information to an array for futher reference.
//!
//! @SYMTestExpectedResults client registration is successful, panics otherwise.
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::RegisterClient()
	{
	TUint clientId = 0;
	RMClientInfo info;
	if(iMaxClientId >  MAX_CLIENTS)
		{
		test.Printf(_L("Reached maximum client allocation. Can't allocate more\n"));
		return;
		}
	ClientName[6] = (TUint8)('0' + iMaxClientId);
	r = lddChan.RegisterClient(clientId, (const TDesC*)&ClientName);
	if(r != KErrNone)
		test.Printf(_L("Register Client failed with %d\n"), r);
	test(r == KErrNone);
	info.iClientId = clientId;
	info.iNumResources = 0;
	for(TUint c = 0; c< MAX_STATIC_RESOURCE_NUM; c++)
		info.iResourceIds[c] = 0;
	iMaxClientId++;
	iMaxClients++;
	r = Clients.Append(info);
	if(r != KErrNone)
		test.Printf(_L("Client Append failed with %d\n"), r);
	test(r == KErrNone);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0574
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests the client deregisteration functionality of resource
//!				manager.It deregisteres a client with the resource manager, calculates
//!				the resource level of each resource that the client was having requirement and
//!				checks the resource level change after client deregistration.
//! @SYMTestActions	 0	Deregister a client from resource manager
//!				1	Calculates the resource level of each resource the client was having requirement. 
//!				2	Checks the resource level change of each resource for correctness.
//!				3	Zeros the clientId stored internally to make sure it is not referenced again. 
//!
//! @SYMTestExpectedResults client deregistration is successful and also the resource level of 
//!							each resource the client was holding the resource level is checked
//!							for correctness, panics otherwise 
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::DeRegisterClient(TUint aClientId)
	{
	r = lddChan.DeRegisterClient(Clients[aClientId].iClientId);
	if(r != KErrNone)
		test.Printf(_L("Client deregistration of %d failed with %d\n"), Clients[aClientId].iClientId, r);
 	test(r == KErrNone);
 	TUint count;
	for(count = 0; count < iMaxStaticResources; count++)
		{
		RMResInfo *pR = &Resources[count];
		NotiInfo* pI = pR->iCondNoti;
		NotiInfo* ptr = NULL;
		//Remove any conditional notification that this client has on resource.
		while(pI != NULL)
			{
			if(pI->iClientId == aClientId)
				{
				ptr = pI;
				pI = pI->iNext;
				LIST_REMOVE(pR->iCondNoti, ptr, iNext, NotiInfo);
				delete ptr;
				}
			else
				pI = pI->iNext;
			}

		//Remove any unconditional notification that this client has on resource.
		pI = pR->iUncondNoti;
		ptr = NULL;
		while(pI != NULL)
			{
			if(pI->iClientId == aClientId)
				{
				ptr = pI;
				pI = pI->iNext;
				LIST_REMOVE(pR->iUncondNoti, ptr, iNext, NotiInfo);
				delete ptr;
				}
			else
				pI = pI->iNext;
			}
		}
	//Remove client level
	TUint res = 0;
	for(count = 0; count < Clients[aClientId].iNumResources; count++)
		{
		res = Clients[aClientId].iResourceIds[count];
		if(res == 0)
			continue;
		for(TUint c = 0; c< iMaxStaticResources; c++)
			{
			if(Resources[c].iResourceId == res)
				{
				res = c;
				break;
				}
			}
		if(Resources[res].iCurrentClient == (TInt)aClientId)
			{
			if(!Resources[res].iUsage)
				{
				Resources[res].iCurrentLevel = Resources[res].iDefaultLevel;
				Resources[res].iCurrentClient = -1;
				Resources[res].iNumClients = 0;
				}
			else if(Resources[res].iSense == ECustom)
				continue;
			else
				{
				TInt maxLevel = KMinTInt;
				TInt id = -1;
				for(SPowerResourceClientLevel* pCL = Resources[res].iLevel; pCL != NULL; pCL = pCL->iNextInList)
					{
					if(pCL->iClientId == aClientId)
						continue;
					if((maxLevel == KMinTInt) || (maxLevel == pCL->iLevel))
						{
						maxLevel = pCL->iLevel;
						id = pCL->iClientId;
						continue;
						}
					if(Resources[res].iSense == EPositive && pCL->iLevel > maxLevel)
						{
						maxLevel = pCL->iLevel;
						id = pCL->iClientId;
						}
					else if(Resources[res].iSense == ENegative && pCL->iLevel < maxLevel)
						{
						maxLevel = pCL->iLevel;
						id = pCL->iClientId;
						}
					}
				if(id == -1)
					{
					Resources[res].iCurrentLevel = Resources[res].iDefaultLevel;
					Resources[res].iCurrentClient = -1;
					Resources[res].iNumClients = 0;
					}
				else
					{
					Resources[res].iCurrentLevel = maxLevel;
					Resources[res].iCurrentClient = id;
					Resources[res].iNumClients--;
					}
				}
			}
			//Remove client list entry from resource
			for(SPowerResourceClientLevel* pCL = Resources[res].iLevel; pCL != NULL; pCL = pCL->iNextInList)
				{
				if(pCL->iClientId == aClientId)
					{
					LIST_REMOVE(Resources[res].iLevel, pCL, iNextInList, SPowerResourceClientLevel);
					delete pCL;
					break;
					}
				}
			}
	//Verify the resource state consistency
	res = 0;
	TInt newState;
	TInt levelOwnerId;
	if(iMaxClients > 1)
		{
		for(TUint id = 0; id < Clients[aClientId].iNumResources; id++)
			{
			res = Clients[aClientId].iResourceIds[id];
			if(res == 0)
			   continue;
			for(TUint c = 0; c< iMaxStaticResources; c++)
				{
				if(Resources[c].iResourceId == res)
					{
					res = c;
					break;
					}
				}
			r = lddChan.GetResourceStateSync(Clients[0].iClientId, Resources[res].iResourceId, ETrue, newState, levelOwnerId);
			if(r != KErrNone)
				test.Printf(_L("GetResourceStateSync returned with %d"), r);
			test(r == KErrNone);
			if(newState != Resources[res].iCurrentLevel)
				test.Printf(_L("newState = %d, Resources[%d].iCurrentLevel = %d"), newState, Resources[res].iResourceId, Resources[res].iCurrentLevel);
			test(newState == Resources[res].iCurrentLevel);
			if(Resources[res].iCurrentClient == -1)
				test(levelOwnerId == -1);
			else if (levelOwnerId != (TInt)Clients[Resources[res].iCurrentClient].iClientId)
				{
				test.Printf(_L("levelOwnerId = 0x%x, iCurrentClient = 0x%x\n"), levelOwnerId, Resources[res].iCurrentClient);
				test(0);
				}
			}
		}
	Clients[aClientId].iClientId = 0;
 	iMaxClients--;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0575
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests the retrieval of client name functionality of resource
//!				manager and compares for correctness. There are negative and positive tests.
//! @SYMTestActions	 If negative testing is enabled then following tests are done
//!				0	Call the API with invalid client Id (calling client Id).
//!				1	Call the API with invalid instance count of calling client Id.
//!				2	Call the API with invalid target client Id.
//!				3	Call the API with invalid instance count of target client Id.
//!				Positive tests
//!				4	Call the API with valid client Ids (both calling and target client ID)
//!
//! @SYMTestExpectedResults  0	API should return with KErrAccessDenied, panics otherwise
//!							 1	API should return with KErrAccessDenied, panics otherwise
//!							 2	API should return with KErrNotFound, panics otherwise
//!							 3	API should return with KErrNotFound, panics otherwise
//!							 4	API should return KErrNone with name updated and also name
//!								is checked for correctness, panics otherwise
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::GetClientName(TUint aClientId)
	{
	ClientName[6] = (TUint8)('0' + aClientId);
	TBuf8<32> name;
	if(NegativeTesting)
		{
		//Pass illegal client Id
		r = lddChan.GetClientName(0, Clients[aClientId].iClientId, (TDes8*)&name);
		test(r == KErrAccessDenied);

		//Pass illegal instance count
		TUint id = Clients[aClientId].iClientId;
		id = id ^ (3<<16);
		r = lddChan.GetClientName(id, Clients[aClientId].iClientId, (TDes8*)&name);
		test(r == KErrAccessDenied);

		//Pass illegal target client id
		r = lddChan.GetClientName(Clients[aClientId].iClientId, iMaxClients, (TDes8*)&name);
		test(r == KErrNotFound);

		//Pass illegal instance count of target client id
		id = id ^ (1<<16);
		r = lddChan.GetClientName(Clients[aClientId].iClientId, 0 ,(TDes8*)&name);
		test(r == KErrNotFound);
		}
	r = lddChan.GetClientName(Clients[aClientId].iClientId, Clients[aClientId].iClientId, (TDes8*)&name);
	if(r != KErrNone)
		test.Printf(_L("GetClientName of ClientId 0x%x returned with %d"), Clients[aClientId].iClientId, r);
	test(r == KErrNone);
	if(name.Compare(ClientName))
		test(0);
	}


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0576
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests the retrieval of client ID functionality of resource
//!				manager and compares for correctness. There are negative and positive tests.
//! @SYMTestActions	 If negative testing is enabled then following tests are done
//!				0	Call the API with invalid client Id (calling client Id).
//!				1	Call the API with invalid instance count of calling client Id.
//!				2	Call the API with client name greater than maximum allowable 
//!					client name (32 characters).
//!				3	Call the API with name not registered with resource manager 
//!					(non-existing name). 
//!				Positive tests
//!				4	Call the API with valid client Id.
//!
//! @SYMTestExpectedResults  0	API should return with KErrAccessDenied, panics otherwise
//!							 1	API should return with KErrAccessDenied, panics otherwise
//!							 2	API should return with KErrTooBig, panics otherwise
//!							 3	API should return with KErrNotFound, panics otherwise
//!							 4	API should return KErrNone with client Id updated and also Id
//!								is checked for correctness, panics otherwise
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::GetClientId(TUint aClientId)
	{
	TUint clientId;
	ClientName[6] = (TUint8)('0' + aClientId);
	if(NegativeTesting)
		{
  		//Pass illegial client Id
		r = lddChan.GetClientId(0, (TDesC8&)ClientName, Clients[aClientId].iClientId);
		test(r == KErrAccessDenied);

		//Pass illegal instance count
		TUint id = Clients[aClientId].iClientId;
		id = id ^ (3<<16);
		r = lddChan.GetClientId(id, (TDesC8&)ClientName, Clients[aClientId].iClientId);
		test(r == KErrAccessDenied);

		TBuf8<50> badName = _L8("Clientnamegreaterthan32characters");
		r = lddChan.GetClientId(Clients[aClientId].iClientId, (TDesC8&)badName, clientId);
		test(r == KErrTooBig);

		ClientName[6] = (TUint8)('0' + iMaxClients + 1);
		r = lddChan.GetClientId(Clients[aClientId].iClientId, (TDesC8&)ClientName, clientId);
		test(r == KErrNotFound);
		}
	ClientName[6] = (TUint8)('0' + aClientId);
	r = lddChan.GetClientId(Clients[aClientId].iClientId, (TDesC8&)ClientName, clientId);
	if(r != KErrNone)
		test.Printf(_L("GetClientId returned with %d"), r);
	test(r == KErrNone);
	if(clientId != Clients[aClientId].iClientId)
		test.Printf(_L("ClientId = 0x%x, Expected ClientId = 0x%x"), clientId, Clients[aClientId].iClientId);
	test(clientId == Clients[aClientId].iClientId);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0577
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests the retrieval of resource ID functionality of resource
//!				manager and compares for correctness. There are negative and positive tests.
//! @SYMTestActions	 If negative testing is enabled then following tests are done
//!				0	Call the API with invalid client Id (calling client Id).
//!				1	Call the API with invalid instance count of calling client Id.
//!				2	Call the API with resource name greater than maximum allowable 
//!					resource name (32 characters).
//!				3	Call the API with name not registered with resource manager 
//!					(non-existing name). 
//!				Positive tests
//!				4	Call the API with valid client Id and resource name.
//!
//! @SYMTestExpectedResults  0	API should return with KErrAccessDenied, panics otherwise
//!							 1	API should return with KErrAccessDenied, panics otherwise
//!							 2	API should return with KErrTooBig, panics otherwise
//!							 3	API should return with KErrNotFound, panics otherwise
//!							 4	API should return KErrNone with resource Id updated and also Id
//!								is checked for correctness, panics otherwise
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::GetResourceId(TUint aResId)
	{
	TUint resId;
	if(NegativeTesting)
		{
		r = lddChan.GetResourceId(iMaxClients, Resources[aResId].iName, resId);
		test(r == KErrAccessDenied);

		//Pass illegal instance count
		TUint id = Clients[iCurrentClientId].iClientId;
		id = id ^ (3<<17);
		r = lddChan.GetResourceId(id, (TDesC8&)Resources[aResId].iName, resId);
		test(r == KErrAccessDenied);

		TBuf8<50> badName = _L8("Resourcenamegreaterthen32characters");
		r = lddChan.GetResourceId(Clients[iCurrentClientId].iClientId, (TDesC8&)badName, resId);
		test(r == KErrTooBig);
		badName = Resources[aResId].iName;
		badName[0] = '0' + 1;
		r = lddChan.GetResourceId(Clients[iCurrentClientId].iClientId, (TDesC8&)badName, resId);
		test(r == KErrNotFound);
		}
	r = lddChan.GetResourceId(Clients[iCurrentClientId].iClientId, (TDesC8&)Resources[aResId].iName, resId);
	if(r != KErrNone)
		test.Printf(_L("Return value of GetResourceId %d"), r);
	test(r == KErrNone);
	if(resId != Resources[aResId].iResourceId)
		test.Printf(_L("resId = %d ... aResId = %d"), resId, Resources[aResId].iResourceId);
	test(resId == Resources[aResId].iResourceId);
	}


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0578
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests the retrieval of resource information of a specified 
//!						resource functionality of resource manager and compares each info for correctness.
//!						There are negative and positive tests.
//! @SYMTestActions	 If negative testing is enabled then following tests are done
//!				0	Call the API with invalid client Id (calling client Id).
//!				1	Call the API with invalid instance count of calling client Id.
//!				2	Call the API with invalid resource id.
//!				Positive tests
//!				3	Call the API with valid client Id and resource id.
//!
//! @SYMTestExpectedResults  0	API should return with KErrAccessDenied, panics otherwise
//!							 1	API should return with KErrAccessDenied, panics otherwise
//!							 2	API should return with KErrNotFound, panics otherwise
//!							 3	API should return KErrNone with resource information updated and also 
//!								each information is checked for correctness, panics otherwise
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::GetResourceInfo(TUint aResId)
	{
	RBuf8 infoBuf;
	infoBuf.Create(sizeof(TPowerResourceInfoV01));
	if(NegativeTesting)
		{
		r = lddChan.GetResourceInfo(iMaxClients-5, aResId+1, (TAny*)(TDes8*)&infoBuf);
		test(r == KErrAccessDenied);

		//Pass illegal instance count
		TUint id = Clients[iCurrentClientId].iClientId;
		id = id ^ (5<<17);
		r = lddChan.GetResourceInfo(id, aResId+1, (TAny*)infoBuf.Ptr());
		test(r == KErrAccessDenied);

		r = lddChan.GetResourceInfo(Clients[iCurrentClientId].iClientId, iMaxStaticResources + 30, (TAny*)(TDes8*)&infoBuf);
		test(r == KErrNotFound);
		r = lddChan.GetResourceInfo(Clients[iCurrentClientId].iClientId, 26, (TAny*)(TDes8*)&infoBuf);
		test(r == KErrNotFound);
		}
	r = lddChan.GetResourceInfo(Clients[iCurrentClientId].iClientId, Resources[aResId].iResourceId, (TAny*)(TDes8*)&infoBuf);
	if(r != KErrNone)
		test.Printf(_L("GetResourceInfo returned with %d"), r);
	test(r == KErrNone);
	TPowerResourceInfoV01 *ptr = (TPowerResourceInfoV01*)infoBuf.Ptr();
	//Compare results.
	test(ptr->iResourceId == Resources[aResId].iResourceId);
	test(ptr->iLatencyGet == Resources[aResId].iLatencyGet);
	test(ptr->iLatencySet == Resources[aResId].iLatencySet);
	test(ptr->iType == Resources[aResId].iType);
	test(ptr->iUsage == Resources[aResId].iUsage);
	test(ptr->iSense == Resources[aResId].iSense);
	test(ptr->iMaxLevel == Resources[aResId].iMaxLevel);
	test(ptr->iMinLevel == Resources[aResId].iMinLevel);
	test(ptr->iDefaultLevel == Resources[aResId].iDefaultLevel);
	if(ptr->iUsage == ESingle && Resources[aResId].iNumClients >1) //Single user resource cannot have more than 1 client.
		test(0);
	infoBuf.Close();
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0579
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests retrieval of number of resources the requested client has
//!						requirement functionality of resource manager and compares with stored information
//!						for correctness.There are negative and positive tests.
//! @SYMTestActions	 If negative testing is enabled then following tests are done
//!				0	Call the API with invalid client Id (calling client Id).
//!				1	Call the API with invalid instance count of calling client Id.
//!				2	Call the API with invalid target client Id. 
//!				3	Call the API with invalid instance count of target client Id.
//!				Positive tests
//!				4	Call the API with valid calling and target client Id.
//!
//! @SYMTestExpectedResults  0	API should return with KErrAccessDenied, panics otherwise
//!							 1	API should return with KErrAccessDenied, panics otherwise
//!							 2	API should return with KErrNotFound, panics otherwise
//!							 3	API should return with KErrNotFound, panics otherwise
//!							 4	API should return KErrNone with number of resources the requested client has 
//!								requirement updated and also is checked for correctness, panics otherwise
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::GetNumResourcesInUseByClient(TInt aClientId)
	{
	TUint numRes;
	if(NegativeTesting)
		{
   		//Pass illegial client Id
		r = lddChan.GetNumResourcesInUseByClient(23, Clients[aClientId].iClientId, numRes);
		test(r == KErrAccessDenied);

		//Pass illegal instance count
		TUint id = Clients[aClientId].iClientId;
		id = id ^ (1<<16);
		r = lddChan.GetNumResourcesInUseByClient(id, Clients[aClientId].iClientId, numRes);
		test(r == KErrAccessDenied);

		//Pass illegal target client id
		r = lddChan.GetNumResourcesInUseByClient(Clients[aClientId].iClientId, iMaxClients, numRes);
		test(r == KErrNotFound);

		//Pass illegal instance count of target client id
		id = id ^ (3<<16);
		r = lddChan.GetNumResourcesInUseByClient(Clients[aClientId].iClientId, id ,numRes);
		test(r == KErrNotFound);
		}

	if(aClientId == -1)
		{
		r = lddChan.GetNumResourcesInUseByClient(Clients[0].iClientId, 0, numRes);
		if(r != KErrNone)
			test.Printf(_L("GetNumResourcesInUseByClient returned with %d"), r);
		test(r == KErrNone);
		if((!iTestingExtendedVersion) && (numRes > MAX_STATIC_RESOURCE_NUM))
		   test(0);
		if(iMaxStaticResources == 0)
			iMaxStaticResources = numRes;
		else 
			test(numRes == (iMaxStaticResources + iMaxStaticDependentResources));
		}
	else
		{
		r = lddChan.GetNumResourcesInUseByClient(Clients[aClientId].iClientId, Clients[aClientId].iClientId, numRes);
		if(r != KErrNone)
			test.Printf(_L("GetNumResourceInUseByClient returned with %d"), r);
		test(r == KErrNone);
		if(numRes != Clients[aClientId].iNumResources)
			test.Printf(_L("numRes = %d, iNumResources = %d"), numRes, Clients[aClientId].iNumResources);
		test(numRes == Clients[aClientId].iNumResources);
   		}
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0580
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests retrieval of information about resources the requested client has
//!						requirement functionality of resource manager and compares with stored information
//!						for correctness.There are negative and positive tests.
//! @SYMTestActions	 If negative testing is enabled then following tests are done
//!				0	Call the API with invalid client Id (calling client Id).
//!				1	Call the API with invalid instance count of calling client Id.
//!				2	Call the API with invalid target client Id. 
//!				3	Call the API with invalid instance count of target client Id.
//!				4	Call the API with null buffer (buffer where the resource information will be updated).
//!				5	Call the API with the number of resource information to be updated as 0 (specifies the 
//!					size of the buffer).
//!				Positive tests
//!				6	Call the API with valid calling and target client Id, buffer and its size.
//!
//! @SYMTestExpectedResults  0	API should return with KErrAccessDenied, panics otherwise
//!							 1	API should return with KErrAccessDenied, panics otherwise
//!							 2	API should return with KErrNotFound, panics otherwise
//!							 3	API should return with KErrNotFound, panics otherwise
//!							 4	API should return with KErrArgument, panics otherwise
//!							 5	API should return with KErrArgument, panics otherwise
//!							 6	API should return KErrNone with resource information about resources the requested
//!								client has requirement updated and also is checked for correctness, panics otherwise
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::GetInfoOnResourcesInUseByClient(TInt aClientId, TUint aNumRes)
	{
	RBuf8 info;
	info.Create(aNumRes * sizeof(TPowerResourceInfoV01));
	if(NegativeTesting)
		{
   		//Pass illegial client Id
		r = lddChan.GetInfoOnResourcesInUseByClient(32, Clients[aClientId].iClientId, aNumRes, (TAny*)(TDes8*)&info);
		test(r == KErrAccessDenied);

		//Pass illegal instance count
		TUint id = Clients[aClientId].iClientId;
		id = id ^ (1<<16);
		r = lddChan.GetInfoOnResourcesInUseByClient(id, Clients[aClientId].iClientId, aNumRes, (TAny*)(TDes8*)&info);
		test(r == KErrAccessDenied);

		//Pass illegal target client id
		r = lddChan.GetInfoOnResourcesInUseByClient(Clients[aClientId].iClientId, iMaxClients, aNumRes, (TAny*)(TDes8*)&info);
		test(r == KErrNotFound);

		//Pass illegal instance count of target client id
		id = id ^ (3<<16);
		r = lddChan.GetInfoOnResourcesInUseByClient(Clients[aClientId].iClientId, id ,aNumRes,(TAny*)(TDes8*)&info);
		test(r == KErrNotFound);

		//Pass null buffer
		r = lddChan.GetInfoOnResourcesInUseByClient(Clients[aClientId].iClientId, Clients[aClientId].iClientId ,aNumRes, (TAny*)NULL);
		test(r == KErrArgument);

		//Pass required resources as 0
		TUint tempRes = 0;
		r = lddChan.GetInfoOnResourcesInUseByClient(Clients[aClientId].iClientId, Clients[aClientId].iClientId, tempRes, (TAny*)(TDes8*)&info);
		test(r == KErrArgument);
		}

	if(aClientId == -1)
		{
		r = lddChan.GetInfoOnResourcesInUseByClient(Clients[0].iClientId, 0, aNumRes, (TAny*)(TDes8*)&info);
		if(r != KErrNone)
			test.Printf(_L("GetInfoOnResourceInUseByClient returned with %d"), r);
		test(r == KErrNone);
		if(aNumRes != (iMaxStaticResources + iMaxStaticDependentResources))
			test.Printf(_L("aNumRes = %d, iMaxStaticResources = %d"), aNumRes, iMaxStaticResources);
		test(aNumRes == (iMaxStaticResources + iMaxStaticDependentResources));

		//Fill in the resource information
		TInt newState, levelOwnerId;
		TPowerResourceInfoV01 *ptr = (TPowerResourceInfoV01*)info.Ptr();

		TUint extCount = 0;
		for(TUint count = 0; count < aNumRes; count++)
			{
			r = lddChan.GetResourceStateSync(Clients[0].iClientId, ptr->iResourceId, ETrue, newState, levelOwnerId);
			if(r != KErrNone)
				test.Printf(_L("GetResourceStateSync failed for ClientId = %d, resourceId = %d with return value %d\n"),  Clients[0].iClientId, count+1, r);
			test(r == KErrNone);
			test.Printf(_L("Info of Resource %d\n"), count+1);
			test.Printf(_L("Resource Id %d\n"), ptr->iResourceId);
			test.Printf(_L("Resource Type %d\n"), ptr->iType);
			test.Printf(_L("Resource Sense %d\n"), ptr->iSense);
			test.Printf(_L("Resource Latency Get %d\n"), ptr->iLatencyGet);
			test.Printf(_L("Resource Latency Set %d\n"), ptr->iLatencySet);
			test.Printf(_L("Resource usage %d\n"), ptr->iUsage);
			test.Printf(_L("Resource MinLevel %d\n"), ptr->iMinLevel);
			test.Printf(_L("Resource MaxLevel %d\n"), ptr->iMaxLevel);
			test.Printf(_L("Resource DefaultLevel %d\n"), ptr->iDefaultLevel);

			if(iTestingExtendedVersion && (ptr->iResourceId & DEPENDENCY_RESOURCE_BIT_MASK))
				{
				iStaticDependencyResources[extCount++] = ptr->iResourceId;
				ptr++;
				continue;
				}
			if(iTestingExtendedVersion && (ptr->iResourceId & DYNAMIC_RESOURCE_BIT_MASK))
				{
				ptr++;
				continue;
				}
			Resources[count].iName.Copy(*ptr->iResourceName);
			Resources[count].iResourceId = ptr->iResourceId;
			Resources[count].iMaxLevel = ptr->iMaxLevel;
			Resources[count].iMinLevel = ptr->iMinLevel;
			Resources[count].iDefaultLevel = ptr->iDefaultLevel;
			Resources[count].iNumClients = 0;
			Resources[count].iCurrentClient = -1;
			Resources[count].iCurrentLevel = newState;
			Resources[count].iType = ptr->iType;
			Resources[count].iSense = ptr->iSense;
			Resources[count].iLatencyGet = ptr->iLatencyGet;
			Resources[count].iLatencySet = ptr->iLatencySet;
			Resources[count].iUsage = ptr->iUsage;
			Resources[count].iUnCondNotiCount = 0;
			Resources[count].iCondNoti = NULL;
			Resources[count].iUncondNoti = NULL;
			Resources[count].iLevel = NULL;
			ptr++;
			}
		iMaxStaticResources -= extCount;
		iMaxStaticDependentResources = extCount;
		info.Close();
		return;
		}
	r = lddChan.GetInfoOnResourcesInUseByClient(Clients[aClientId].iClientId, Clients[aClientId].iClientId, aNumRes, (TAny*)&info);
	if(aNumRes != Clients[aClientId].iNumResources)
		{
		test.Printf(_L("Expected Resource Num = %d, Returned = %d\n"), Clients[aClientId].iNumResources, aNumRes);
		test(0);
		}
	if(aNumRes == 0)
		{
		test((r == KErrArgument) || (r == KErrNone));
		info.Close();
		return;
		}
	else
	   test(r == KErrNone);
	TPowerResourceInfoV01 *ptr = (TPowerResourceInfoV01*)info.Ptr();
	for(TUint count = 0; count < Clients[aClientId].iNumResources; count++)
		{
		 if(Clients[aClientId].iResourceIds[count] == 0)
			continue;
		 TUint c;
		 for(c = 0; c < Clients[aClientId].iNumResources; c++)
			{
			if(Clients[aClientId].iResourceIds[c] == ptr->iResourceId)
				break;
			}
		if(c == Clients[aClientId].iNumResources)
			test(0);
		//Find the resource from resource list
		for(c = 0; c < iMaxStaticResources; c++)
			{
			if(Resources[c].iResourceId == ptr->iResourceId)
				break;
			}
		if(c == iMaxStaticResources)
			test(0);
		test(Resources[c].iResourceId == ptr->iResourceId);
		test(Resources[c].iMaxLevel == ptr->iMaxLevel);
		test(Resources[c].iMinLevel == ptr->iMinLevel);
		test(Resources[c].iDefaultLevel == ptr->iDefaultLevel);
		test(Resources[c].iType == ptr->iType);
		test(Resources[c].iSense == ptr->iSense);
		test(Resources[c].iLatencyGet == ptr->iLatencyGet);
		test(Resources[c].iLatencySet == ptr->iLatencySet);
		test(Resources[c].iUsage == ptr->iUsage);
		test(!Resources[c].iName.Compare(*ptr->iResourceName));
		ptr++;
		}
	info.Close();
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0581
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests retrieval of number of clients holding requirement on 
//!						the requested resource functionality of resource manager and compares with stored
//!						information for correctness.There are negative and positive tests.
//! @SYMTestActions	 If negative testing is enabled then following tests are done
//!				0	Call the API with invalid client Id (calling client Id).
//!				1	Call the API with invalid instance count of calling client Id.
//!				2	Call the API with invalid resource Id. 
//!				Positive tests
//!				3	Call the API with valid calling client Id and resource Id. 
//!
//! @SYMTestExpectedResults  0	API should return with KErrAccessDenied, panics otherwise
//!							 1	API should return with KErrAccessDenied, panics otherwise
//!							 2	API should return with KErrNotFound, panics otherwise
//!							 3	API should return KErrNone with number of clients holding requirement on 
//!								the requested resource updated and also is checked for correctness, panics otherwise
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::GetNumClientsUsingResource(TUint aClientId, TUint aResId)
	{
	TUint clientNum = 0;
	if(NegativeTesting)
		{
 		//Pass illegial client Id
		r = lddChan.GetNumClientsUsingResource(32, 1, clientNum);
		test(r == KErrAccessDenied);

		//Pass illegal instance count
		TUint id = Clients[aClientId].iClientId;
		id = id ^ (1<<16);
		r = lddChan.GetNumClientsUsingResource(id, 1, clientNum);
		test(r == KErrAccessDenied);

		//Invalid resource id
		r = lddChan.GetNumClientsUsingResource(Clients[aClientId].iClientId, iMaxStaticResources+40, clientNum);
		test(r == KErrNotFound);
		}
	if((TInt)aResId == -1)
		r = lddChan.GetNumClientsUsingResource(Clients[aClientId].iClientId, 0, clientNum);
	else
		r = lddChan.GetNumClientsUsingResource(Clients[aClientId].iClientId, Resources[aResId].iResourceId, clientNum);
	if(r != KErrNone)
		test.Printf(_L("GetNumClientsUsingResource for client 0x%x failed with %d"), Clients[aClientId].iClientId, r);
	test(r==KErrNone);
	if((TInt)aResId == -1)
		{
		if(clientNum != (TUint)(Clients.Count() + 1))
			test.Printf(_L("ClientNum = %d, Expected clientNum = %d"), clientNum, Clients.Count()+1);
		test(clientNum == (TUint)(Clients.Count() + 1));
		}
	else
		{
		test(Resources[aResId].iNumClients == clientNum);
		if(!Resources[aResId].iUsage && clientNum > 1) //Single user resource cannot have more that one client
			test(0);
		}
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0582
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests retrieval of information about clients holding requirement on 
//!						the passed resource functionality of resource manager and compares with stored information
//!						for correctness.There are negative and positive tests.
//! @SYMTestActions	 If negative testing is enabled then following tests are done
//!				0	Call the API with invalid client Id (calling client Id).
//!				1	Call the API with invalid instance count of calling client Id.
//!				2	Call the API with invalid resource Id. 
//!				3	Call the API with null buffer (buffer where the resource information will be updated).
//!				4	Call the API with the number of resource information to be updated as 0 (specifies the 
//!					size of the buffer).
//!				Positive tests
//!				5	Call the API with valid calling and target client Id, buffer and its size.
//!
//! @SYMTestExpectedResults  0	API should return with KErrAccessDenied, panics otherwise
//!							 1	API should return with KErrAccessDenied, panics otherwise
//!							 2	API should return with KErrNotFound, panics otherwise
//!							 3	API should return with KErrArgument, panics otherwise
//!							 4	API should return with KErrArgument, panics otherwise
//!							 5	API should return KErrNone with resource information about clients holding requirement
//!								on the passed resource and also is checked for correctness, panics otherwise
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::GetInfoOnClientsUsingResource(TUint aResId, TUint aNumCli)
	{
	RBuf8 info;
	info.Create(aNumCli * sizeof(TPowerClientInfoV01));
	if(NegativeTesting)
		{
 		//Pass illegial client Id
		r = lddChan.GetInfoOnClientsUsingResource(2, 1, aNumCli, (TAny*)(TDes8*)&info);
		test(r == KErrAccessDenied);

		//Pass illegal instance count
		TUint id = Clients[iCurrentClientId].iClientId;
		id = id ^ (1<<16);
		r = lddChan.GetInfoOnClientsUsingResource(id, 1, aNumCli, (TAny*)(TDes8*)&info);
		test(r == KErrAccessDenied);

		//Invalid resource id
		r = lddChan.GetInfoOnClientsUsingResource(Clients[iCurrentClientId].iClientId, iMaxStaticResources+40, aNumCli, (TAny*)(TDes8*)&info);
		test(r == KErrNotFound);

		//Pass null buffer
		r = lddChan.GetInfoOnClientsUsingResource(Clients[iCurrentClientId].iClientId, 1 ,aNumCli, (TAny*)NULL);
		test(r == KErrArgument);

		//Pass required resources as 0
		TUint tempCli = 0;
		r = lddChan.GetInfoOnClientsUsingResource(Clients[iCurrentClientId].iClientId, 1 ,tempCli, (TAny*)(TDes8*)&info);
		test(r == KErrArgument);
		}
	if((TInt)aResId == -1)
		r = lddChan.GetInfoOnClientsUsingResource(Clients[iCurrentClientId].iClientId, 0, aNumCli, (TAny*)(TDes8*)&info);
	else
		r = lddChan.GetInfoOnClientsUsingResource(Clients[iCurrentClientId].iClientId, Resources[aResId].iResourceId, aNumCli, (TAny*)(TDes8*)&info);
	if(r == KErrArgument)
		{
		if(aResId != 0)
			test(Resources[aResId].iNumClients == 0);
		info.Close();
	  	return;
		}
	test(r == KErrNone);
	TPowerClientInfoV01 *ptr = (TPowerClientInfoV01*)info.Ptr();
	if((TInt)aResId == -1)
		{
		test(aNumCli == (TUint)(Clients.Count() + 1));
		TUint c = 0;
		for(TUint count = 0; count < aNumCli; count++)
			{
			//Skip comparision of first client as that will be PowerController.
			if(ptr->iClientId == iPowerControllerId)
				{
				ptr++;
				continue;
				}
			for(c = 0; c< iMaxClients; c++)
				{
				if(ptr->iClientId == Clients[c].iClientId)
					break;
				}
			if(c == iMaxClients)
			   test(0);
			ptr++;
			}
		}
	else
		{
		if(aNumCli != Resources[aResId].iNumClients)
			test.Printf(_L("aNumCli = %d, Expected numClients = %d\n"), aNumCli, Resources[aResId].iNumClients);
		test(aNumCli == Resources[aResId].iNumClients);
		//Compare results
		SPowerResourceClientLevel *level = Resources[aResId].iLevel;
		TUint c = 0;
		for(TUint count = 0; count < aNumCli; count++)
			{
			SPowerResourceClientLevel *pL = Resources[aResId].iLevel;
   			for(c =0;c<aNumCli;c++)
				{
				if(Clients[pL->iClientId].iClientId == ptr->iClientId)
					break;
				pL = pL->iNextInList;
				}
			if(c == aNumCli)
				{
				test.Printf(_L("Client Id %d is not in the resource clientlevel list\n"), Clients[level->iClientId].iClientId);
				test(0);
				}
			level = level->iNextInList;
			ptr++;
			}
		}
	info.Close();
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0583
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests preallocation of memory for resource manager internal
//!						structure.There are negative and positive tests.
//! @SYMTestActions	 If negative testing is enabled then following tests are done
//!				0	Call the API with invalid client Id 
//!				1	Call the API with invalid instance count of client Id.
//!				Positive tests
//!				2	Call the API with valid client Id.
//!
//! @SYMTestExpectedResults  0	API should return with KErrAccessDenied, panics otherwise
//!							 1	API should return with KErrAccessDenied, panics otherwise
//!							 2	API should return with KErrNone, panic otherwise. Really cannot
//!								test this for correctness.
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::AllocReserve(TUint aClientId)
	{
	if(NegativeTesting)
		{
  		//Pass illegial client Id
		r = lddChan.AllocReserve(11, 0, 3);
		test(r == KErrAccessDenied);

		//Pass illegal instance count
		TUint id = Clients[aClientId].iClientId;
		id = id ^ (1<<16);
		r = lddChan.AllocReserve(id, 0, 0);
		test(r == KErrAccessDenied);

		}
	r = lddChan.AllocReserve(Clients[iCurrentClientId].iClientId, 1, 0);
	if(r != KErrNone)
		test.Printf(_L("Alloc Reserve failed with %d"), r);
	test(r == KErrNone);
	}

//This function validates the conditional and unconditional notification for the 
//specified resource state change.
void TestRM::CheckNotification(TUint aResId, TInt newState)
	{
	if(newState == Resources[aResId].iCurrentLevel)
		return;
	//Return if the newState is in decreasing order with respect to sense.
	if(Resources[aResId].iUsage == EShared && Resources[aResId].iCurrentClient != -1)
		{
		if(Resources[aResId].iSense == EPositive)
			{
			if(newState <= Resources[aResId].iCurrentLevel && Resources[aResId].iCurrentClient != iCurrentClientId)
				return;
			}
		else
			{
			if(newState >= Resources[aResId].iCurrentLevel && Resources[aResId].iCurrentClient != iCurrentClientId)
				return;
			}
		}
	TUint notificationUnCon = Resources[aResId].iUnCondNotiCount;
	TUint notificationCon =0;
	for(NotiInfo* info = Resources[aResId].iCondNoti; info != NULL; info = info->iNext)
		{
		if((info->iDirection && (info->iPreviousLevel < info->iThreshold) && (newState >= info->iThreshold)) || 
			(!info->iDirection && (info->iPreviousLevel > info->iThreshold) && (newState <= info->iThreshold)))
			notificationCon++;
		info->iPreviousLevel = newState;
		}
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, notificationUnCon, notificationCon);
	if(r != KErrNone)
		test.Printf(_L("Check Notifications failed with %d"), r);
	test(r == KErrNone);
	}

//This function updates the client level.This will be used by other functions for validation.
void TestRM::AddClientLevel(TUint aResId, TInt newState)
	{
	SPowerResourceClientLevel *pCL = NULL;
	if(Resources[aResId].iUsage == EShared)
		{
		for(pCL = Resources[aResId].iLevel;pCL != NULL; pCL = pCL->iNextInList)
			{
			if((TInt)pCL->iClientId == iCurrentClientId)
				{
				pCL->iLevel = newState;
				return;
				}
			}
		pCL = new SPowerResourceClientLevel;
		test(pCL != NULL);
		pCL->iClientId = iCurrentClientId;
		pCL->iResourceId = Resources[aResId].iResourceId;
		pCL->iLevel = newState;
		LIST_PUSH(Resources[aResId].iLevel, pCL, iNextInList);
		Resources[aResId].iNumClients++;
		}
	else
		{
		if(Resources[aResId].iCurrentClient == -1)
			{
			pCL = new SPowerResourceClientLevel;
			test(pCL != NULL);
			pCL->iClientId = iCurrentClientId;
			pCL->iResourceId = Resources[aResId].iResourceId;
			pCL->iLevel = newState;
			LIST_PUSH(Resources[aResId].iLevel, pCL, iNextInList);
			Resources[aResId].iNumClients++;
			}
		else
			{
			SPowerResourceClientLevel* pCL = Resources[aResId].iLevel;
			pCL->iLevel = newState;
			}
		}
	}

//This function updates the current level and client information in corresponding resource array.
void TestRM::UpdateClientInformation(TUint aResId, TInt aNewState)
	{
	if(Resources[aResId].iCurrentClient == -1)
		{
		Resources[aResId].iCurrentLevel = aNewState;
		Resources[aResId].iCurrentClient = iCurrentClientId;
		return;
		}
	if(!Resources[aResId].iUsage)
		{
		Resources[aResId].iCurrentLevel = aNewState;
		return;
		}
	if(Resources[aResId].iSense == EPositive)
		{
		if(aNewState > Resources[aResId].iCurrentLevel)
			{
			Resources[aResId].iCurrentLevel = aNewState;
			Resources[aResId].iCurrentClient = iCurrentClientId;
			}
		else if(Resources[aResId].iCurrentClient == iCurrentClientId)
			{
			SPowerResourceClientLevel *pCL = NULL;
			for(pCL = Resources[aResId].iLevel;pCL != NULL; pCL = pCL->iNextInList)
				{
				if(pCL->iLevel > aNewState)
					{
					Resources[aResId].iCurrentLevel = pCL->iLevel;
					Resources[aResId].iCurrentClient = pCL->iClientId;
					return;
					}
				}
			Resources[aResId].iCurrentLevel = aNewState;
			Resources[aResId].iCurrentClient = iCurrentClientId;
			}
			return;
		}
	 if(Resources[aResId].iSense == ENegative)
		{
		if(aNewState < Resources[aResId].iCurrentLevel)
			{
			Resources[aResId].iCurrentLevel = aNewState;
			Resources[aResId].iCurrentClient = iCurrentClientId;
			}
		else if(Resources[aResId].iCurrentClient == iCurrentClientId)
			{
			SPowerResourceClientLevel *pCL = NULL;
			for(pCL = Resources[aResId].iLevel;pCL != NULL; pCL = pCL->iNextInList)
				{
				if(pCL->iLevel < aNewState)
					{
					Resources[aResId].iCurrentLevel = pCL->iLevel;
					Resources[aResId].iCurrentClient = pCL->iClientId;
					return;
					}
				}
				Resources[aResId].iCurrentLevel = aNewState;
				Resources[aResId].iCurrentClient = iCurrentClientId;
			}
		}
		return;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0584
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests change resource state functionality of resource manager
//!						by changing the state of the resource to random value between resource minimum
//!						and maximum value synchronously.This function will add the client level if required 
//!						and update resource information and will check notification for correctness.There are
//!						postive and negative tests.
//! @SYMTestActions	 If negative testing is enabled then following tests are done
//!				0	Call the API with invalid client Id 
//!				1	Call the API with invalid instance count of client Id.
//!				2	Call the API with invalid resource Id
//!				Positive tests
//!				3	Call the API with valid client and resource Id.
//!
//! @SYMTestExpectedResults  0	API should return with KErrAccessDenied, panics otherwise
//!							 1	API should return with KErrAccessDenied, panics otherwise
//!							 2	API should return with KErrNotFound, panic otherwise. 
//!							 3  API should return with KErrNone, panic otherwise.
//!								This also checks for notifications revceived as a result of this
//!								resource change and checks for correctness.
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::ChangeResourceStateSync(TUint aResId)
	{
	TInt newState = 0;
	if(NegativeTesting)
		{
		//Pass illegial client Id
		r = lddChan.ChangeResourceStateSync(434224, Resources[aResId].iResourceId, newState);
		test(r == KErrAccessDenied);

		//Pass illegal instance count
		TUint id = Clients[iCurrentClientId].iClientId;
		id = id ^ (1<<16);
		r = lddChan.ChangeResourceStateSync(id, Resources[aResId].iResourceId, newState);
		test(r == KErrAccessDenied);

		//Invalid resource id
		r = lddChan.ChangeResourceStateSync(Clients[iCurrentClientId].iClientId, iMaxStaticResources+40, newState);
		test(r == KErrNotFound);

		r = lddChan.ChangeResourceStateSync(Clients[iCurrentClientId].iClientId, 26, newState);
		test(r == KErrNotFound);
		}
	TInt maxLevel = Resources[aResId].iMaxLevel;
	TInt minLevel = Resources[aResId].iNumClients? Resources[aResId].iCurrentLevel : Resources[aResId].iMinLevel;
	//Toggle current state for binary resources
	if(!Resources[aResId].iType)
		newState = !Resources[aResId].iCurrentLevel;
	else if (Resources[aResId].iType == EMultiLevel)
		{
		TInt diff = Abs(maxLevel - minLevel);
		if(Resources[aResId].iSense == EPositive)
			{
			if(minLevel == maxLevel)
				newState = maxLevel - Math::Random() % diff;
			else
				newState = minLevel + Math::Random() % diff;
			}
		else
			{
			if(minLevel == maxLevel)
				newState = maxLevel + Math::Random() % diff;
			else
				newState = minLevel - Math::Random() % diff;
			}
		}
	TInt reqState = newState;
	r = lddChan.ChangeResourceStateSync(Clients[iCurrentClientId].iClientId, Resources[aResId].iResourceId, newState);
	if(r == KErrAccessDenied)
		return;
	if(r != KErrNone)
		test.Printf(_L("Synchronous resource change returned with %d"), r);
	test(r == KErrNone);
	if(newState != reqState)
		test.Printf(_L("NewState = %d, Expected state = %d"), newState, reqState);
	if(!Resources[aResId].iUsage)
		test(newState == reqState);
	CheckNotification(aResId, reqState);
	AddClientLevel(aResId, reqState);
	UpdateClientInformation(aResId, reqState);
	TUint c = 0;
	for(c = 0; c< Clients[iCurrentClientId].iNumResources; c++)
		{
		if(Clients[iCurrentClientId].iResourceIds[c] == Resources[aResId].iResourceId)
		   return;
		}
	Clients[iCurrentClientId].iResourceIds[c] = Resources[aResId].iResourceId;
	Clients[iCurrentClientId].iNumResources++;
	return;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0585
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests change resource state functionality of resource manager
//!						by changing the state of the resource to random value between resource minimum
//!						and maximum value asynchronously.This function will add the client level if required 
//!						and update resource information and will check notification for correctness.This 
//!						also tests the cancellation of asynchronous function by immediately cancelling the
//!						operation after requesting resource state change. This is taken care in the driver.
//!						There are postive and negative tests.
//! @SYMTestActions	 If negative testing is enabled then following tests are done
//!				0	Call the API with invalid client Id 
//!				1	Call the API with invalid instance count of client Id.
//!				2	Call the API with invalid resource Id
//!				Positive tests
//!				3	Call the API with valid client and resource Id.
//!
//! @SYMTestExpectedResults  0	API should return with KErrAccessDenied, panics otherwise
//!							 1	API should return with KErrAccessDenied, panics otherwise
//!							 2	API should return with KErrNotFound, panic otherwise. 
//!							 3  API should return with KErrNone or if cancellation of this
//!								API is tested then will return with KErrCancel, panic otherwise.
//!								This also checks for notifications received as a result of this
//!								resource change and checks for correctness.
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::ChangeResourceStateAsync(TUint aResId, TBool aReqCancel)
	{
	TRequestStatus resChange;
	TInt newState = 0;
	if(NegativeTesting)
		{
		//Pass illegial client Id
		lddChan.ChangeResourceStateAsync(434224, Resources[aResId].iResourceId, newState, resChange);
		User::WaitForRequest(resChange);
		test(resChange.Int() == KErrAccessDenied);

		//Pass illegal instance count
		TUint id = Clients[iCurrentClientId].iClientId;
		id = id ^ (1<<16);
		lddChan.ChangeResourceStateAsync(id, Resources[aResId].iResourceId, newState, resChange);
		User::WaitForRequest(resChange);
		test(resChange.Int() == KErrAccessDenied);

		//Invalid resource id
		lddChan.ChangeResourceStateAsync(Clients[iCurrentClientId].iClientId, iMaxStaticResources+40, newState, resChange);
		User::WaitForRequest(resChange);
		test(resChange.Int() == KErrNotFound);

		lddChan.ChangeResourceStateAsync(Clients[iCurrentClientId].iClientId, 19, newState, resChange);
		User::WaitForRequest(resChange);
		test(resChange.Int() == KErrNotFound);
		}
	TInt maxLevel = Resources[aResId].iMaxLevel;
	TInt minLevel = (Resources[aResId].iCurrentClient != -1)? Resources[aResId].iCurrentLevel : Resources[aResId].iMinLevel;
	//Check if the resource is positive
	if(!Resources[aResId].iType)
		newState = !Resources[aResId].iCurrentLevel;
	else if (Resources[aResId].iType == EMultiLevel)
		{
		TInt diff = Abs(maxLevel - minLevel);
		if( diff == 0)
			diff = Abs(Resources[aResId].iMaxLevel - Resources[aResId].iMinLevel);
		if(Resources[aResId].iSense == EPositive)
			{
			if(minLevel == maxLevel)
				newState = maxLevel - Math::Random() % diff;
			else
				newState = minLevel + Math::Random() % diff;
			}
		else
			{
			if(minLevel == maxLevel)
				newState = maxLevel + Math::Random() % diff;
			else
				newState = minLevel - Math::Random() % diff;
			}
		}
	TInt reqState = newState;
	//Long latency resource
	lddChan.ChangeResourceStateAsync(Clients[iCurrentClientId].iClientId, Resources[aResId].iResourceId, newState, resChange, aReqCancel);
	User::WaitForRequest(resChange);
	if(aReqCancel && (resChange.Int() != KErrNone))
		{
		test(resChange.Int() == KErrCancel || resChange.Int() == KErrCompletion);
		return;
		}
	if(resChange.Int() == KErrAccessDenied)
		return;
	if(!Resources[aResId].iUsage)
		test(newState == reqState);
	CheckNotification(aResId, reqState);
	AddClientLevel(aResId, reqState);
	UpdateClientInformation(aResId, reqState);
	TUint c = 0;
	for(c = 0; c< Clients[iCurrentClientId].iNumResources; c++)
		{
		if(Clients[iCurrentClientId].iResourceIds[c] == Resources[aResId].iResourceId)
			return;
		}
	Clients[iCurrentClientId].iResourceIds[c] = Resources[aResId].iResourceId;
	Clients[iCurrentClientId].iNumResources++;
	return;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0586
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests synchronous version of get resource state functionality of 
//!						resource manager by getting the state of the resource checks for correctness.
//!						There are positive and negative tests.
//! @SYMTestActions	 If negative testing is enabled then following tests are done
//!				0	Call the API with invalid client Id 
//!				1	Call the API with invalid instance count of client Id.
//!				2	Call the API with invalid resource Id
//!				Positive tests
//!				3	Call the API with valid client and resource Id.
//!
//! @SYMTestExpectedResults  0	API should return with KErrAccessDenied, panics otherwise
//!							 1	API should return with KErrAccessDenied, panics otherwise
//!							 2	API should return with KErrNotFound, panic otherwise. 
//!							 3  API should return with KErrNone and also the state and Owner Id are checked
//!								for correctness, panic otherwise.
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::GetResourceStateSync(TUint aResId)
	{
	static TBool Cached;
	TInt state = 0, levelOwnerId = 0;
	if(NegativeTesting)
		{
		//Pass illegial client Id
		r = lddChan.GetResourceStateSync(4342241, Resources[aResId].iResourceId, Cached, state, levelOwnerId);
		test(r == KErrAccessDenied);

		//Pass illegal instance count
		TUint id = Clients[iCurrentClientId].iClientId;
		id = id ^ (1<<30);
		r = lddChan.GetResourceStateSync(id, Resources[aResId].iResourceId, Cached, state, levelOwnerId);
		test(r == KErrAccessDenied);

		//Invalid resource id
		r = lddChan.GetResourceStateSync(Clients[iCurrentClientId].iClientId, iMaxStaticResources+40, Cached, state, levelOwnerId);
		test(r == KErrNotFound);

		r = lddChan.GetResourceStateSync(Clients[iCurrentClientId].iClientId, 20, Cached, state, levelOwnerId);
		test(r == KErrNotFound);
		}
	r = lddChan.GetResourceStateSync(Clients[iCurrentClientId].iClientId, Resources[aResId].iResourceId, Cached, state, levelOwnerId);
	test(r == KErrNone);
	test(state == Resources[aResId].iCurrentLevel);
	if(Resources[aResId].iCurrentClient == -1)
		test(levelOwnerId == -1);
	else if (levelOwnerId != (TInt)Clients[Resources[aResId].iCurrentClient].iClientId)
		{
		test.Printf(_L("Expected ClientId = 0x%x, Returned ClientId = 0x%x\n"), Resources[aResId].iCurrentClient, levelOwnerId);
		test(0);
		}
	Cached = !Cached;
	return;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0587
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests get resource state functionality of resource manager
//!						by getting the state of the resource asynchronously and checking for correctness.
//!						This also tests the cancellation of asynchronous function by immediately cancelling the
//!						operation after requesting get resource state. This is taken care in the driver.
//!						There are positive and negative tests.
//! @SYMTestActions	 If negative testing is enabled then following tests are done
//!				0	Call the API with invalid client Id 
//!				1	Call the API with invalid instance count of client Id.
//!				2	Call the API with invalid resource Id
//!				Positive tests
//!				3	Call the API with valid client and resource Id.
//!
//! @SYMTestExpectedResults  0	API should return with KErrAccessDenied, panics otherwise
//!							 1	API should return with KErrAccessDenied, panics otherwise
//!							 2	API should return with KErrNotFound, panic otherwise. 
//!							 3  API should return with KErrNone or if cancellation of this
//!								API is tested then will return with KErrCancel, panic otherwise.
//!								This also checks the updated level and owner Id for correctness, 
//!								panics otherwise.
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::GetResourceStateAsync(TUint aResId, TBool aReqCancel)
	{
	static TBool Cached;
	TRequestStatus resGet;
	TInt state, levelOwnerId;
	if(NegativeTesting)
		{
		//Pass illegial client Id
		lddChan.GetResourceStateAsync(4342241, Resources[aResId].iResourceId, Cached, resGet, state, levelOwnerId);
		User::WaitForRequest(resGet);
		test(resGet.Int() == KErrAccessDenied);

		//Pass illegal instance count
		TUint id = Clients[iCurrentClientId].iClientId;
		id = id ^ (1<<30);
		lddChan.GetResourceStateAsync(id, Resources[aResId].iResourceId, Cached, resGet, state, levelOwnerId);
		User::WaitForRequest(resGet);
		test(resGet.Int() == KErrAccessDenied);

		//Invalid resource id
		lddChan.GetResourceStateAsync(Clients[iCurrentClientId].iClientId, iMaxStaticResources+48, Cached, resGet, state, levelOwnerId);
		User::WaitForRequest(resGet);
		test(resGet.Int() == KErrNotFound);

		lddChan.GetResourceStateAsync(Clients[iCurrentClientId].iClientId, 20, Cached, resGet, state, levelOwnerId);
		User::WaitForRequest(resGet);
		test(resGet.Int() == KErrNotFound);
		}
	lddChan.GetResourceStateAsync(Clients[iCurrentClientId].iClientId, Resources[aResId].iResourceId, Cached, resGet, state, levelOwnerId, aReqCancel);
	User::WaitForRequest(resGet);
	if(aReqCancel && (resGet.Int() != KErrNone))
		{
		test(resGet.Int() == KErrCancel || resGet.Int() == KErrCompletion);
		return;
		}
	test(state == Resources[aResId].iCurrentLevel);
	if(Resources[aResId].iCurrentClient == -1)
		test(levelOwnerId == -1);
	else if (levelOwnerId != (TInt)Clients[Resources[aResId].iCurrentClient].iClientId)
			{
		test.Printf(_L("Expected ClientId = 0x%x, Returned ClientId = 0x%x\n"), Resources[aResId].iCurrentClient, levelOwnerId);
		test(0);
			}
	Cached = !Cached;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0588
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests unconditional notification request functionality of resource manager.
//! @SYMTestActions		Call the API with valid client and resource Id.
//!
//! @SYMTestExpectedResults	API should return with KErrNone,	panics otherwise.
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::RequestNotification(TUint aResId)
	{
	//If unconditional notification is already queued for this client then dont request another one. 
	for(NotiInfo *pN = Resources[aResId].iUncondNoti; pN != NULL; pN = pN->iNext)
		{
		if((TInt)pN->iClientId == iCurrentClientId)
			return;
		}
	r = lddChan.RequestNotification(Clients[iCurrentClientId].iClientId, Resources[aResId].iResourceId);
	if(r != KErrNone)
		test.Printf(_L("Request Notification returned with %d"), r);
	test(r == KErrNone);
	//Add to resource list
	NotiInfo *info = new NotiInfo;
	test(info != NULL);
	info->iClientId = iCurrentClientId;
	LIST_PUSH(Resources[aResId].iUncondNoti, info, iNext);
	Resources[aResId].iUnCondNotiCount++;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0589
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests conditional notification request functionality of resource manager.
//!						Threshold and direction are chosen randomly for each resource based on the resource information.
//! @SYMTestActions		Call the API with valid client and resource Id.
//!
//! @SYMTestExpectedResults	API should return with KErrNone,	panics otherwise.
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::RequestNotificationCon(TUint aResId)
	{
	//Allow only one notification per client.
	static TBool direction;
	TInt threshold = direction;
	for(NotiInfo *pN = Resources[aResId].iCondNoti; pN != NULL; pN = pN->iNext)
		{
		if((TInt)pN->iClientId == iCurrentClientId)
			return;
		}
	if(Resources[aResId].iType)
		{
		if(Resources[aResId].iSense == EPositive)
			{
			threshold = Math::Random() % Resources[aResId].iMaxLevel;
			if(threshold < Resources[aResId].iMinLevel)
				threshold += Resources[aResId].iMinLevel;
			}
		else if(Resources[aResId].iSense == ENegative)
			{
			threshold = Math::Random() % Resources[aResId].iMinLevel;
			if(threshold < Resources[aResId].iMaxLevel)
				threshold += Resources[aResId].iMaxLevel;
			}
		}
	r = lddChan.RequestNotification(Clients[iCurrentClientId].iClientId, Resources[aResId].iResourceId, direction, threshold);
	if(r != KErrNone)
		test.Printf(_L("Request Notification returned with %d for direction = %d, threshold = %d"), r, direction, threshold);
	test(r == KErrNone);
	NotiInfo *info = new NotiInfo;
	test(info != NULL);
	info->iClientId = iCurrentClientId;
	info->iThreshold = threshold;
	info->iDirection = direction;
	info->iPreviousLevel = Resources[aResId].iCurrentLevel;
	LIST_PUSH(Resources[aResId].iCondNoti, info, iNext);
	direction = !direction;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0590
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests cancellation of notification functionality of resource manager.
//! @SYMTestActions		Call the API with valid client and resource Id.
//!
//! @SYMTestExpectedResults	API should return with KErrCancel, panics otherwise.
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::CancelNotification(TUint aResId, TBool Cond)
	{
	RMResInfo *pR = &Resources[aResId];
	TBool found = EFalse;
	if(Cond)
		{
		//Remove any conditional notification this client has on resource.
		for(NotiInfo* pI = pR->iCondNoti; pI != NULL; pI = pI->iNext)
			{
			if((TInt)pI->iClientId == iCurrentClientId)
				{
				LIST_REMOVE(pR->iCondNoti, pI, iNext, NotiInfo);
				delete pI;
				found = ETrue;
				break;
				}
			}
		}
	else
		{
		//Remove any unconditional notification this client has on resource.
		for(NotiInfo* pI = pR->iUncondNoti; pI != NULL; pI = pI->iNext)
			{
			if((TInt)pI->iClientId == iCurrentClientId)
				{
				LIST_REMOVE(pR->iUncondNoti, pI, iNext, NotiInfo);
				pR->iUnCondNotiCount--;
				delete pI;
				found = ETrue;
				break;
				}
			}
		}
	if(found)
		{
		r = lddChan.CancelNotification(Clients[iCurrentClientId].iClientId, Resources[aResId].iResourceId, Cond);
		if(r != KErrCancel)
			test.Printf(_L("CancelNotification Clients %d, return value = %d"), iCurrentClientId, r);
		test(r == KErrCancel);
		}
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0591
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests client registration and deregistration API of resource manager.
//!						There are positive and negative tests.
//! @SYMTestActions		0	Call the client registration API with valid client name to register
//!						1	Call the client name updation API with valid client Id to get the client name
//!						2	Call the client name updation API with invalid client id.
//!						3	Call the client registration API with client name greater than maximum
//!							allowable name length (32 characters)
//!						4	Call the client deregistration API by passing invalid client Id.
//!						5	Call the client deregistration API by passing invalid instance count.
//!						6	Call the client deregistration API by passing valid client Id.
//!
//! @SYMTestExpectedResults   0	API should return with KErrNone, panics otherwise.
//!							  1	API should return with KErrNone and updated name is checked for 
//!								correctness, panics otherwise.
//!							  2	API should return with KErrAccessDenied, panics otherwise.
//!							  3 API should return with KErrTooBig, panics otherwise.
//!							  4 API should return with KErrNotFound, panics otherwise.
//!							  5 API should return with KErrNotFound, panics otherwise.
//!							  6 API should return with KErrNone, panics otherwise.
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::ValidateClient(TUint aNumClients, TOwnerType aContext)
	{
	TInt r = KErrNone;
	TBuf8<32> ClientName;
	ClientName.Zero();
	ClientName.Append(_L8("Clients?"));
	TUint clientId[MAX_CLIENTS];
	if(aNumClients > MAX_CLIENTS)
		return;
	TUint c;
	for(c = 0; c < aNumClients; c++)
		{
		ClientName[7] = (TUint8)('0' + c);
		r = lddChan.RegisterClient(clientId[c], (const TDesC*)&ClientName, aContext);
		if(r != KErrNone)
			{
			test.Printf(_L("Client registration failed with %d"), r);
			test(0);
			}
		}

	//Validate Client
	TBuf8<32> aName;

	for(c = 0; c < aNumClients; c++)
		{
		ClientName[7] = (TUint8)('0' + c);
		r = lddChan.GetClientName(clientId[c], clientId[c], (TDes8*)&aName);
		if(r != KErrNone)
			{
			test.Printf(_L("GetClientName API failed with error %d"), r);
			test(0);
			}
		r = aName.Compare(ClientName);
		if(r != KErrNone)
			{
			test.Printf(_L("Client Name is not as expected"));
			test(0);
			}
		}
	//Invalid tests
	ClientName[7] = (TUint8)('0' + aNumClients+1);
	r = lddChan.GetClientName(aNumClients, clientId[0], &aName);
	if(r != KErrAccessDenied)
		{
		test.Printf(_L("RM allows illegal clients"));
		test(0);
		}

	//Long filename
	TBuf8<50> name;
	name.Zero();
	name.Append(_L8("RegisteringClientNameGreaterThan32Characters"));
	TUint id =0;
	r = lddChan.RegisterClient(id,  (const TDesC*)&name, aContext);
	if(r != KErrTooBig)
		{
		test.Printf(_L("RM allows big names !!!"));
		test(0);
		}
	test.Printf(_L("Client Deregistration"));
	//Deregistration of non-existing client
	id = 0;
	r = lddChan.DeRegisterClient(id);
	if(r != KErrNotFound)
		{
		test.Printf(_L("RM allows invalid client ID deregistration!!!"));
		test(0);
		}

	//Get client Name by passing invalid client Id (changing a bit in instance count)
	id = clientId[0] ^ (1<<16);
	r	= lddChan.DeRegisterClient(id);
	if(r != KErrNotFound)
		{
		test.Printf(_L("RM allows invalid client ID deregistation!!!"));
		test(0);
		}

	//Deregister the client registered at the start of this function
	for(c = 0; c < aNumClients; c++)
		{
		r = lddChan.DeRegisterClient(clientId[c]);
		if(r != KErrNone)
			{
			test.Printf(_L("Deregistration of client id 0x%x failed"), clientId[c]);
			test(0);
			}
		}
	return;
	}

#ifdef PRM_ENABLE_EXTENDED_VERSION
//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0597
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests dynamic resources with dependency.
//! @SYMTestActions		0	Register clients
//!						1	Register dynamic resource with dependency
//!						2	Establish dependency between resources
//!						3	Register notifications
//!						4	Check dependency information for correctness
//!						5	Change Resource State of each resource
//!						6	Get state of the resources and verify them for correctness
//!						7	Check notification count for correctness
//!						8	Deregister dependency between resources
//!						9	Deregister client level 
//!						10	Deregister dynamic resource with dependency
//!						11	Deregister clients
//!
//! @SYMTestExpectedResults   0	 API should return with KErrNone, panics otherwise.
//!							  1	 API should return with KErrNone, panics otherwise.
//!							  2	 API should return with KErrNone, panics otherwise.
//!							  3  API should return with KErrNone, panics otherwise.
//!							  4  API should return with KErrNone, panics otherwise.
//!							  5  API should return with KErrNone, panics otherwise.
//!							  6  API should return with KErrNone, panics otherwise.
//!							  7  API should return with KErrNone, panics otherwise.
//!							  8  API should return with KErrNone, panics otherwise.
//!							  9  API should return with KErrNone, panics otherwise.
//!							  10 API should return with KErrNone, panics otherwise.
//!							  11 API should return with KErrNone, panics otherwise.
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
/*This tests dynamic dependency resource. It also creates a dependency between static resource.
  Below is the dependency tree
	ResourceA <----------------> ResourceD <------------->ResourceE <--------------> ResourceC 
									|						  |
									|						  |
									|						  |	
									|						  |	
									|						  |
									|						  |
								ResourceF				   ResourceG <-------------> Resource H <------->Resource I	
																						¦	(Dynamic)		(Dynamic)
																						¦
																						¦
																						¦
																					 Resource J <-------->Resource K
																							(Dynamic)		  (Dynamic)
*/
void TestRM::TestDynamicResourceDependency()
	{
	TInt state;
	TRequestStatus req;
	SResourceDependencyInfo info1, info2;
	SResourceDependencyInfo sResDepInfo;
	RArray<SResourceDependencyInfo>depResArray;

	TUint dynamicDepResId[4];

	test.Next(_L("Testing Dynamic + static resource dependency"));
	RmTest.RegisterClient(); /* Register Client 1 */
	
	//Register dependency resource
	dynamicDepResId[0] = 5;
	r = lddChan.RegisterDynamicResource(Clients[0].iClientId, dynamicDepResId[0]);
	test(r == KErrNone);

	info1.iResourceId = iStaticDependencyResources[5];
	info1.iDependencyPriority = 3;

	info2.iResourceId = dynamicDepResId[0];
	info2.iDependencyPriority = 2;

	r = lddChan.RegisterResourceDependency(Clients[0].iClientId, info1, info2);
	test(r == KErrNone);

	//Check for correctness of dependency resource information
	sResDepInfo.iResourceId = iStaticDependencyResources[3]; 
	sResDepInfo.iDependencyPriority = 1;
	depResArray.Append(sResDepInfo);
	sResDepInfo.iResourceId = dynamicDepResId[0]; 
	sResDepInfo.iDependencyPriority = 2;
	depResArray.Append(sResDepInfo);
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, iStaticDependencyResources[5], 2, &depResArray[0]);

	depResArray[0].iResourceId = iStaticDependencyResources[5]; 
	depResArray[0].iDependencyPriority = 3;
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, dynamicDepResId[0], 1, &depResArray[0]);

	//Change Static dependency resource to -50
	RmTest.RegisterClient(); /* Register Client 2 */
	state = -50;
	lddChan.ChangeResourceStateAsync(Clients[1].iClientId, iStaticDependencyResources[0], state, req);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);

	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[0], -50, Clients[1].iClientId, EFalse);
	GetExtendedResStateAndVerify(iStaticDependencyResources[1], -11, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 1, iStaticDependencyResources[0], EFalse);
	GetExtendedResStateAndVerify(iStaticDependencyResources[3], 13, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3], EFalse);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 73, iStaticDependencyResources[3]);
	GetExtendedResStateAndVerify(dynamicDepResId[0], 80, iStaticDependencyResources[5]);

	//Register dynamic dependency resource I
	dynamicDepResId[1] = 6;
	r = lddChan.RegisterDynamicResource(Clients[0].iClientId, dynamicDepResId[1]);
	test(r == KErrNone);

	//Register dynamic dependency resource J
	dynamicDepResId[2] = 7;
	r = lddChan.RegisterDynamicResource(Clients[0].iClientId, dynamicDepResId[2]);
	test(r == KErrNone);

	//Register dynamic dependency resource K
	dynamicDepResId[3] = 8;
	r = lddChan.RegisterDynamicResource(Clients[0].iClientId, dynamicDepResId[3]);
	test(r == KErrNone);

	RmTest.RegisterClient(); /* Register Client3 */
	RmTest.RegisterClient(); /* Register Client4 */

	//Register notifications
	r = lddChan.RequestNotification(Clients[1].iClientId, iStaticDependencyResources[5]);
	test(r == KErrNone);
	r = lddChan.RequestNotification(Clients[2].iClientId, dynamicDepResId[0]);
	test(r == KErrNone);
	r = lddChan.RequestNotification(Clients[1].iClientId, dynamicDepResId[1]);
	test(r == KErrNone);
	r = lddChan.RequestNotification(Clients[2].iClientId, dynamicDepResId[2]);
	test(r == KErrNone);
	r = lddChan.RequestNotification(Clients[1].iClientId, dynamicDepResId[3]);
	test(r == KErrNone);

	//Create depedency between H and I
	info1.iResourceId = dynamicDepResId[0];
	info1.iDependencyPriority = 1;

	info2.iResourceId = dynamicDepResId[1];
	info2.iDependencyPriority = 1;

	//Register dependency between resource H and I
	r = lddChan.RegisterResourceDependency(Clients[0].iClientId, info1, info2);
	test(r == KErrNone);
	//Validate dependency information
	depResArray[0].iResourceId = dynamicDepResId[1]; 
	depResArray[0].iDependencyPriority = 1;
	depResArray[1].iResourceId = iStaticDependencyResources[5]; 
	depResArray[1].iDependencyPriority = 3;
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, dynamicDepResId[0], 2, &depResArray[0]);

	depResArray[0].iResourceId = dynamicDepResId[0]; 
	depResArray[0].iDependencyPriority = 1;
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, dynamicDepResId[1], 1, &depResArray[0]);

	//Create depedency between H and J
	info1.iResourceId = dynamicDepResId[0];
	info1.iDependencyPriority = 1;

	info2.iResourceId = dynamicDepResId[2];
	info2.iDependencyPriority = 2;

	//Register dependency between resource H and J
	r = lddChan.RegisterResourceDependency(Clients[0].iClientId, info1, info2);
	test(r == KErrNone);

	depResArray[0].iResourceId = dynamicDepResId[1]; 
	depResArray[0].iDependencyPriority = 1;
	depResArray[1].iResourceId = dynamicDepResId[2]; 
	depResArray[1].iDependencyPriority = 2;
	sResDepInfo.iResourceId = iStaticDependencyResources[5]; 
	sResDepInfo.iDependencyPriority = 3;	
	depResArray.Append(sResDepInfo);
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, dynamicDepResId[0], 3, &depResArray[0]);

	depResArray[0].iResourceId = dynamicDepResId[0]; 
	depResArray[0].iDependencyPriority = 1;
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, dynamicDepResId[2], 1, &depResArray[0]);

	// Check if the priority for a new dependency is already existing.
	//Create dependency between J and K
	info1.iResourceId = dynamicDepResId[2];
	info1.iDependencyPriority = 1;

	info2.iResourceId = dynamicDepResId[3];
	info2.iDependencyPriority = 1;

	//Register dependency between resource J and K
	r = lddChan.RegisterResourceDependency(Clients[0].iClientId, info1, info2);
	test(r == KErrAlreadyExists);
	
	//Create depedency between J and K
	info1.iResourceId = dynamicDepResId[2];
	info1.iDependencyPriority = 1;

	info2.iResourceId = dynamicDepResId[3];
	info2.iDependencyPriority = 2;

	//Register dependency between resource J and K
	r = lddChan.RegisterResourceDependency(Clients[0].iClientId, info1, info2);
	test(r == KErrNone);
	
	depResArray[0].iResourceId = dynamicDepResId[2]; 
	depResArray[0].iDependencyPriority = 1;
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, dynamicDepResId[3], 1, &depResArray[0]);

	depResArray[0].iResourceId = dynamicDepResId[0]; 
	depResArray[0].iDependencyPriority = 1;
	depResArray[1].iResourceId = dynamicDepResId[3]; 
	depResArray[1].iDependencyPriority = 2;
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, dynamicDepResId[2], 2, &depResArray[0]);
	
	RmTest.RegisterClient(); /* Client5 registration */
	//Change H to 85
	r = lddChan.ChangeResourceStateSync(Clients[2].iClientId, dynamicDepResId[0], 85);
	test(r == KErrNone);

	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[0], -50, Clients[1].iClientId, EFalse);
	GetExtendedResStateAndVerify(iStaticDependencyResources[1], -11, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 1, iStaticDependencyResources[0], EFalse);
	GetExtendedResStateAndVerify(iStaticDependencyResources[3], 13, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3], EFalse);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 71, dynamicDepResId[0]);
	GetExtendedResStateAndVerify(dynamicDepResId[0], 85, Clients[2].iClientId);
	GetExtendedResStateAsyncAndVerify(dynamicDepResId[1], 1, dynamicDepResId[0]);
	GetExtendedResStateAsyncAndVerify(dynamicDepResId[2], 18, dynamicDepResId[0], EFalse);
	GetExtendedResStateAndVerify(dynamicDepResId[3], 1, dynamicDepResId[2]);

	//Check notifications
	r = lddChan.CheckNotifications(iStaticDependencyResources[5], 1, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(dynamicDepResId[0], 1, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(dynamicDepResId[1], 1, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(dynamicDepResId[2], 1, 0);
	test(r == KErrNone);

	//Change K to  1
	state= 1;
	lddChan.ChangeResourceStateAsync(Clients[2].iClientId, dynamicDepResId[3], state, req);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);

	GetExtendedResStateAsyncAndVerify(dynamicDepResId[2], 18, dynamicDepResId[0], EFalse);
	GetExtendedResStateAsyncAndVerify(dynamicDepResId[3], 1, Clients[2].iClientId, EFalse);

	//Check notifications
	r = lddChan.CheckNotifications(dynamicDepResId[2], 0, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(dynamicDepResId[3], 0, 0);
	test(r == KErrNone);

	//Change J to 12
	r = lddChan.ChangeResourceStateSync(Clients[2].iClientId, dynamicDepResId[2],12);
	test(r == KErrNone);

	GetExtendedResStateAsyncAndVerify(dynamicDepResId[2], 12, Clients[2].iClientId, EFalse);

	//Check notifications
	r = lddChan.CheckNotifications(dynamicDepResId[0], 0, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(dynamicDepResId[2], 1, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(dynamicDepResId[3], 0, 0);
	test(r == KErrNone);
	
	//Change H to 90
	TRequestStatus reqSet;
	state = 90;
	lddChan.CheckParallelExecutionForChangeResState(Clients[2].iClientId,
											 dynamicDepResId[0],state,
											 5, 0, reqSet);
	User::WaitForRequest(reqSet);
	test(reqSet.Int() == KErrNone);
	
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[0], -50, Clients[1].iClientId, EFalse);
	GetExtendedResStateAndVerify(iStaticDependencyResources[1], -11, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 1, iStaticDependencyResources[0], EFalse);
	GetExtendedResStateAndVerify(iStaticDependencyResources[3], 13, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3], EFalse);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 69, dynamicDepResId[0]);
	GetExtendedResStateAndVerify(dynamicDepResId[0], 90, Clients[2].iClientId);
	GetExtendedResStateAsyncAndVerify(dynamicDepResId[1], 1, dynamicDepResId[0]);
	GetExtendedResStateAsyncAndVerify(dynamicDepResId[2], 11, dynamicDepResId[0], EFalse);
	GetExtendedResStateAndVerify(dynamicDepResId[3], 1, Clients[2].iClientId);

	//Check notifications
	r = lddChan.CheckNotifications(iStaticDependencyResources[5], 1, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(dynamicDepResId[0], 1, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(dynamicDepResId[1], 0, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(dynamicDepResId[2], 1, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(dynamicDepResId[3], 0, 0);
	test(r == KErrNone);

	state = 9;
	lddChan.ChangeResourceStateAsync(Clients[1].iClientId, dynamicDepResId[2],  state, req);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);

	GetExtendedResStateAndVerify(dynamicDepResId[0], 90, Clients[2].iClientId);
	GetExtendedResStateAsyncAndVerify(dynamicDepResId[1], 1, dynamicDepResId[0]);
	GetExtendedResStateAsyncAndVerify(dynamicDepResId[2], 9, Clients[1].iClientId, EFalse);
	GetExtendedResStateAndVerify(dynamicDepResId[3], 1, Clients[2].iClientId);

	//Check notifications
	r = lddChan.CheckNotifications(dynamicDepResId[0], 0, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(dynamicDepResId[1], 0, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(dynamicDepResId[2], 1, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(dynamicDepResId[3], 0, 0);
	test(r == KErrNone);

	//Change D to 50
	state = 50;
	lddChan.ChangeResourceStateAsync(Clients[1].iClientId, iStaticDependencyResources[0], state, req);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);

	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[0], 50, Clients[1].iClientId, EFalse);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -12, iStaticDependencyResources[0], EFalse);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 1, iStaticDependencyResources[0], EFalse);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[3], 16, iStaticDependencyResources[0], EFalse);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3], EFalse);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[5], 67, iStaticDependencyResources[3], EFalse);
	GetExtendedResStateAndVerify(dynamicDepResId[0], 90, Clients[2].iClientId);
	GetExtendedResStateAsyncAndVerify(dynamicDepResId[1], 1, dynamicDepResId[0]);
	GetExtendedResStateAsyncAndVerify(dynamicDepResId[2], 9, Clients[1].iClientId, EFalse);
	GetExtendedResStateAndVerify(dynamicDepResId[3], 1, Clients[2].iClientId);

	//Check notifications
	r = lddChan.CheckNotifications(iStaticDependencyResources[5], 1, 0);
	test(r == KErrNone);
	//DeRegister dependency between J and K
	r = lddChan.DeRegisterResourceDependency(Clients[0].iClientId, dynamicDepResId[3], dynamicDepResId[2]);
	test(r == KErrNone);

	RmTest.CheckForDependencyInformation(Clients[0].iClientId, dynamicDepResId[3], 0, &depResArray[0]);

	depResArray[0].iResourceId = dynamicDepResId[0]; 
	depResArray[0].iDependencyPriority = 1;
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, dynamicDepResId[2], 1, &depResArray[0]);

	//Change J t0 13
	r = lddChan.ChangeResourceStateSync(Clients[1].iClientId, dynamicDepResId[2], 13);
	test(r == KErrNone);

	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[0], 50, Clients[1].iClientId, EFalse);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -12, iStaticDependencyResources[0], EFalse);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 1, iStaticDependencyResources[0], EFalse);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[3], 16, iStaticDependencyResources[0], EFalse);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3], EFalse);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[5], 67, iStaticDependencyResources[3], EFalse);
	GetExtendedResStateAndVerify(dynamicDepResId[0], 90, Clients[2].iClientId);
	GetExtendedResStateAsyncAndVerify(dynamicDepResId[1], 1, dynamicDepResId[0]);
	GetExtendedResStateAsyncAndVerify(dynamicDepResId[2], 12, Clients[2].iClientId, EFalse);
	GetExtendedResStateAndVerify(dynamicDepResId[3], 1, Clients[2].iClientId);
	//Check notifications
	r = lddChan.CheckNotifications(dynamicDepResId[2], 1, 0);
	test(r == KErrNone);
	/* Remove client level from resource 7 */
	r = lddChan.DeRegisterClientLevelFromResource(Clients[2].iClientId, dynamicDepResId[3]);
	test(r == KErrNone);

	GetExtendedResStateAndVerify(dynamicDepResId[3], 0, -1);
	//Check notifications
	r = lddChan.CheckNotifications(dynamicDepResId[3], 1, 0);
	test(r == KErrNone);

	r = lddChan.DeRegisterDynamicResource(Clients[0].iClientId, dynamicDepResId[3], NULL);
	test(r == KErrNone);
	//Check notifications
	r = lddChan.CheckNotifications(dynamicDepResId[3], 1, 0);
	test(r == KErrNone);

	//Deregister dependency between H and J
	r = lddChan.DeRegisterResourceDependency(Clients[0].iClientId, dynamicDepResId[2], dynamicDepResId[0]);
	test(r == KErrNone);

	RmTest.CheckForDependencyInformation(Clients[0].iClientId, dynamicDepResId[2], 0, &depResArray[0]);

	depResArray[0].iResourceId = dynamicDepResId[1]; 
	depResArray[0].iDependencyPriority = 1;
	depResArray[1].iResourceId = iStaticDependencyResources[5]; 
	depResArray[1].iDependencyPriority = 3;
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, dynamicDepResId[0], 2, &depResArray[0]);

	/* Remove client level from resource 7 */
	r = lddChan.DeRegisterClientLevelFromResource(Clients[1].iClientId, dynamicDepResId[2]);
	test(r == KErrNone);

	GetExtendedResStateAndVerify(dynamicDepResId[2], 12, Clients[2].iClientId);
	//Check notifications
	r = lddChan.CheckNotifications(dynamicDepResId[2], 0, 0);
	test(r == KErrNone);

	r = lddChan.DeRegisterClient(Clients[2].iClientId);
	test(r == KErrNone);

	GetExtendedResStateAsyncAndVerify(dynamicDepResId[2], 19, -1, EFalse);

	//Deregister dependency between G and H
	r = lddChan.DeRegisterResourceDependency(Clients[1].iClientId, iStaticDependencyResources[5], dynamicDepResId[0]);
	test(r == KErrNone);

	depResArray[0].iResourceId = dynamicDepResId[1]; 
	depResArray[0].iDependencyPriority = 1;
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, dynamicDepResId[0], 1, &depResArray[0]);

	depResArray[0].iResourceId = iStaticDependencyResources[3]; 
	depResArray[0].iDependencyPriority = 1;
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, iStaticDependencyResources[5], 1, &depResArray[0]);

	GetExtendedResStateAndVerify(dynamicDepResId[0], 75, -1);
	GetExtendedResStateAsyncAndVerify(dynamicDepResId[1], 0, dynamicDepResId[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[5], 67, iStaticDependencyResources[3], EFalse);

	//Deregister dependency between I and H
	r = lddChan.DeRegisterResourceDependency(Clients[1].iClientId, dynamicDepResId[1], dynamicDepResId[0]);
	test(r == KErrNone);

	RmTest.CheckForDependencyInformation(Clients[0].iClientId, dynamicDepResId[0], 0, &depResArray[0]);

	RmTest.CheckForDependencyInformation(Clients[0].iClientId, dynamicDepResId[1], 0, &depResArray[0]);

	GetExtendedResStateAndVerify(dynamicDepResId[0], 75, -1);
	GetExtendedResStateAsyncAndVerify(dynamicDepResId[1], 0, -1);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[5], 67, iStaticDependencyResources[3], EFalse);
	//Check notifications
	r = lddChan.CheckNotifications(iStaticDependencyResources[5], 0, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(dynamicDepResId[1], 1, 0);
	test(r == KErrNone);

	r = lddChan.DeRegisterDynamicResource(Clients[0].iClientId, dynamicDepResId[2], NULL);
	test(r == KErrNone);
	r = lddChan.DeRegisterDynamicResource(Clients[0].iClientId, dynamicDepResId[1], NULL);
	test(r == KErrNone);
	//Check notifications
	r = lddChan.CheckNotifications(dynamicDepResId[1], 1, 0);
	test(r == KErrNone);
	r = lddChan.DeRegisterDynamicResource(Clients[0].iClientId, dynamicDepResId[0], NULL);
	test(r == KErrNone);
	r = lddChan.DeRegisterClient(Clients[1].iClientId);
	test(r == KErrNone);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[0], -100, -1, EFalse);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -10, iStaticDependencyResources[0], EFalse);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 0, iStaticDependencyResources[0], EFalse);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[3], 10, iStaticDependencyResources[0], EFalse);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 1, iStaticDependencyResources[3], EFalse);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[5], 75, iStaticDependencyResources[3], EFalse);
	r = lddChan.DeRegisterClient(Clients[3].iClientId);
	test(r == KErrNone);

	r = lddChan.DeRegisterClient(Clients[0].iClientId);
	test(r == KErrNone);

	r = lddChan.DeRegisterClient(Clients[4].iClientId);
	test(r == KErrNone);

	depResArray.Close();
	return;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0596
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests dynamic resources which does not support dependency
//! @SYMTestActions		0	Register clients
//!						1	Register dynamic resource
//!						2	Register notifications
//!						3	Change Resource State of each static resource with dependency
//!						4	Get state of the resource and check for correctness
//!						5	Check notification count for correctness
//!						6	Deregister client level	
//!						7	Deregister dynamic resource
//!						8	Deregister clients
//!
//! @SYMTestExpectedResults   0	API should return with KErrNone, panics otherwise.
//!							  1	API should return with KErrNone, panics otherwise.
//!							  2	API should return with KErrNone, panics otherwise.
//!							  3 API should return with KErrNone, panics otherwise.
//!							  4 API should return with KErrNone, panics otherwise.
//!							  5 API should return with KErrNone, panics otherwise.
//!							  6 API should return with KErrNone, panics otherwise.
//!							  7 API should return with KErrNone, panics otherwise.
//!							  8 API should return with KErrNone, panics otherwise.
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::TestDynamicResource()
	{
	TInt state;
	TRequestStatus req;
	TUint dynamicResId[4];

	test.Next(_L("Testing dynamic resource"));
	//Register client 1
	RmTest.RegisterClient(); 
	//Register client 2
	RmTest.RegisterClient(); 
	//Register client 3
	RmTest.RegisterClient();
	
	NegativeTesting = EFalse;
	dynamicResId[0] = 1;
	//Register dynamic resource 1
	r = lddChan.RegisterDynamicResource(Clients[0].iClientId, dynamicResId[0]);
	test(r == KErrNone);
	//Deregister dynamic resource with different client id 
	r = lddChan.DeRegisterDynamicResource(Clients[1].iClientId, dynamicResId[0], NULL);
	test(r == KErrAccessDenied);
	dynamicResId[1] = 2;
	//Register dynamic resource 2
	r = lddChan.RegisterDynamicResource(Clients[1].iClientId, dynamicResId[1]);
	test(r == KErrNone);
	RmTest.GetNumResourcesInUseByClient(iCurrentClientId);
	RmTest.GetInfoOnResourcesInUseByClient(iCurrentClientId, iMaxStaticResources);
	
	TUint numClients;
	r = lddChan.GetNumClientsUsingResource(Clients[0].iClientId, dynamicResId[0], numClients);
	test(r == KErrNone);
	test(numClients == 0);

	r = lddChan.RequestNotification(Clients[1].iClientId, dynamicResId[0]);
	test(r == KErrNone);
	//Register client 4
	RmTest.RegisterClient(); 
	r = lddChan.RequestNotification(Clients[2].iClientId, dynamicResId[0], 1, 1);
	test(r == KErrNone);
	//Change state of dynamic resource 1 and verify
	state = 1;
	lddChan.ChangeResourceStateAsync(Clients[0].iClientId, dynamicResId[0], state, req);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);

	r = lddChan.CheckNotifications(dynamicResId[0], 1, 1);
	test(r == KErrNone);

	GetExtendedResStateAsyncAndVerify(dynamicResId[0], 1, Clients[0].iClientId, EFalse);

	r = lddChan.GetNumClientsUsingResource(Clients[0].iClientId, dynamicResId[0], numClients);
	test(r == KErrNone);
	test(numClients == 1);
	//Change state of dynamic resource 1.
	r = lddChan.ChangeResourceStateSync(Clients[1].iClientId, dynamicResId[0], 0);
	test(r == KErrAccessDenied);
	//Deregister dynamic resource 1 and set the resource to 1.
	state = 1;
	r = lddChan.DeRegisterDynamicResource(Clients[0].iClientId, dynamicResId[0], &state);
	test(r == KErrNone);

	r = lddChan.CheckNotifications(dynamicResId[0], 1, 1);
	test(r == KErrNone);

	r = lddChan.GetNumClientsUsingResource(Clients[0].iClientId, dynamicResId[0], numClients);
	test(r == KErrNotFound);
	//Register client 5
	RmTest.RegisterClient(); 

	GetExtendedResStateAndVerify(dynamicResId[1], -5, -1);

	r = lddChan.RequestNotification(Clients[1].iClientId, dynamicResId[1]);
	test(r == KErrNone);
	 
	r = lddChan.RequestNotification(Clients[2].iClientId, dynamicResId[1], 0, -8);
	test(r == KErrNone);
	//Change state of dynamic resource 1 and verify
	state = -7;
	lddChan.ChangeResourceStateAsync(Clients[2].iClientId, dynamicResId[1], state, req);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);

	r = lddChan.CheckNotifications(dynamicResId[1], 1, 0);
	test(r == KErrNone);

	GetExtendedResStateAndVerify(dynamicResId[1], -7, Clients[2].iClientId);
	//Register client 6
	RmTest.RegisterClient(); 
	//Register client 7
	RmTest.RegisterClient();
	//Change state of dynamic resource 2 and verify
	state = -9;
	lddChan.ChangeResourceStateAsync(Clients[3].iClientId, dynamicResId[1], state, req);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);

	r = lddChan.CheckNotifications(dynamicResId[1], 1, 1);
	test(r == KErrNone);

	GetExtendedResStateAsyncAndVerify(dynamicResId[1], -9, Clients[3].iClientId, EFalse);
	//Change state of dynamic resource 1 and verify
	state = -10;
	lddChan.ChangeResourceStateAsync(Clients[4].iClientId, dynamicResId[1], state, req);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);

	r = lddChan.CheckNotifications(dynamicResId[1], 1, 0);
	test(r == KErrNone);

	GetExtendedResStateAsyncAndVerify(dynamicResId[1], -10, Clients[4].iClientId, EFalse);

	lddChan.ChangeResourceStateSync(Clients[5].iClientId, dynamicResId[1], state);
	test(r == KErrNone);

	r = lddChan.CheckNotifications(dynamicResId[1], 0, 0);
	test(r == KErrNone);

	GetExtendedResStateAsyncAndVerify(dynamicResId[1], -10, Clients[4].iClientId, EFalse);
	//Change state of dynamic resource 1 and verify
	state = -6;
	lddChan.ChangeResourceStateSync(Clients[6].iClientId, dynamicResId[1], state);
	test(r == KErrNone);

	r = lddChan.CheckNotifications(dynamicResId[1], 0, 0);
	test(r == KErrNone);

	GetExtendedResStateAsyncAndVerify(dynamicResId[1], -10, Clients[4].iClientId, EFalse);
	
	r = lddChan.DeRegisterClientLevelFromResource(Clients[4].iClientId, dynamicResId[1]);
	test(r == KErrNone);

	GetExtendedResStateAsyncAndVerify(dynamicResId[1], -10, Clients[5].iClientId, EFalse);

	r = lddChan.DeRegisterClientLevelFromResource(Clients[5].iClientId, dynamicResId[1]);
	test(r == KErrNone);

	r = lddChan.CheckNotifications(dynamicResId[1], 1, 0);
	test(r == KErrNone);
	GetExtendedResStateAsyncAndVerify(dynamicResId[1], -9, Clients[3].iClientId, EFalse);
	//Deregister client 4
	r = lddChan.DeRegisterClient(Clients[3].iClientId);
	test(r == KErrNone);

	r = lddChan.CheckNotifications(dynamicResId[1], 1, 0);
	test(r == KErrNone);
	GetExtendedResStateAsyncAndVerify(dynamicResId[1], -7, Clients[2].iClientId, EFalse);

	r = lddChan.DeRegisterClientLevelFromResource(Clients[2].iClientId, dynamicResId[1]);
	test(r == KErrNone);

	r = lddChan.CheckNotifications(dynamicResId[1], 1, 0);
	test(r == KErrNone);
	GetExtendedResStateAsyncAndVerify(dynamicResId[1], -6, Clients[6].iClientId, EFalse);

	r = lddChan.DeRegisterClientLevelFromResource(Clients[6].iClientId, dynamicResId[1]);
	test(r == KErrNone);

	r = lddChan.CheckNotifications(dynamicResId[1], 1, 0);
	test(r == KErrNone);
	GetExtendedResStateAsyncAndVerify(dynamicResId[1], -5, -1, EFalse);
	//Deregister dynamic resource 2
	r = lddChan.DeRegisterDynamicResource(Clients[1].iClientId, dynamicResId[1], NULL);
	test(r == KErrNone);

	r = lddChan.CheckNotifications(dynamicResId[1], 1, 1);
	test(r == KErrNone);
	//Register dynamic resource 3
	dynamicResId[2] = 3;
	r = lddChan.RegisterDynamicResource(Clients[2].iClientId, dynamicResId[2]);
	test(r == KErrNone);
	//Register dynamic resource 4
	dynamicResId[3] = 4;
	r = lddChan.RegisterDynamicResource(Clients[6].iClientId, dynamicResId[3]);
	test(r == KErrNone);
	//Change state of dynamic resource 3 to 0
	r = lddChan.ChangeResourceStateSync(Clients[4].iClientId, dynamicResId[2], 0);
	test(r == KErrNone);
	GetExtendedResStateAndVerify(dynamicResId[2], 0, Clients[4].iClientId);
	//Change state of dynamic resource 3 to 1
	r = lddChan.ChangeResourceStateSync(Clients[5].iClientId, dynamicResId[2], 1);
	test(r == KErrNone);
	GetExtendedResStateAndVerify(dynamicResId[2], 0, Clients[4].iClientId);
	//Deregister client 5
	r = lddChan.DeRegisterClient(Clients[4].iClientId);
	test(r == KErrNone);
	GetExtendedResStateAndVerify(dynamicResId[2], 1, Clients[5].iClientId);
	//Deregister dynamic resource 3
	r = lddChan.DeRegisterDynamicResource(Clients[2].iClientId, dynamicResId[2], NULL);
	test(r == KErrInUse);
	//Deregister client 6
	r = lddChan.DeRegisterClient(Clients[5].iClientId);
	test(r == KErrNone);
	GetExtendedResStateAndVerify(dynamicResId[2], 1, -1);
	//Deregister dynamic resource 3
	r = lddChan.DeRegisterDynamicResource(Clients[2].iClientId, dynamicResId[2], NULL);
	test(r == KErrNone);
	//Change state of dynamic resource 4 to 15
	r = lddChan.ChangeResourceStateSync(Clients[6].iClientId, dynamicResId[3], 15);
	test(r == KErrNone);
	GetExtendedResStateAndVerify(dynamicResId[3], 15, Clients[6].iClientId);
	//Change state of resource and try to deregister the resource while the change is taking place
	state = 17;
	lddChan.ChangeResStateAndDeRegisterDynamicRes(Clients[6].iClientId, dynamicResId[3], state, req);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);
	test(state == 17);
	GetExtendedResStateAndVerify(dynamicResId[3], 17, Clients[6].iClientId);
	//Deregister dynamic resource 4 with some other client which is not owner
	r = lddChan.DeRegisterDynamicResource(Clients[2].iClientId, dynamicResId[3], NULL);
	test(r == KErrAccessDenied);
	//Deregister dynamic resource 4
	r = lddChan.DeRegisterDynamicResource(Clients[6].iClientId, dynamicResId[3], NULL);
	test(r == KErrNone);
	//Deregister client 7
	r = lddChan.DeRegisterClient(Clients[6].iClientId);
	test(r == KErrNone);
	//Deregister client 3
	r = lddChan.DeRegisterClient(Clients[2].iClientId);
	test(r == KErrNone);
	//Deregister client 2
	r = lddChan.DeRegisterClient(Clients[1].iClientId);
	test(r == KErrNone);
	//Deregister client 1
	r = lddChan.DeRegisterClient(Clients[0].iClientId);
	test(r == KErrNone);
	Clients.Close();
	}

//This function gets extended resource state synchronously and verifies for correctness
void TestRM::GetExtendedResStateAndVerify(TUint aResId, TInt aState, TInt aLevelOwnerId)
	{
	static TBool cached = ETrue;
	TInt state;
	TInt levelOwnerId;
	r = lddChan.GetResourceStateSync(Clients[0].iClientId, aResId, cached, state, levelOwnerId);
	test(r == KErrNone);
	test(state == aState);
	test(levelOwnerId == aLevelOwnerId);
	return;
	}

//This function gets extended resource state asynchronously and verifies for correctness
void TestRM::GetExtendedResStateAsyncAndVerify(TUint aResId, TInt aState, TInt aLevelOwnerId, TBool aReqCancel)
	{
	static TBool cached = ETrue;
	TRequestStatus resGet;
	TInt levelOwnerId;
	TInt state;
	lddChan.GetResourceStateAsync(Clients[0].iClientId, aResId, cached, resGet, state, levelOwnerId, aReqCancel);
	User::WaitForRequest(resGet);
	if(aReqCancel && (resGet.Int() != KErrNone))
		{
		test((resGet.Int() == KErrCompletion) || (resGet.Int() == KErrCancel));
		return;
		}
	test(resGet.Int() == KErrNone);
	test(state == aState);
	test(levelOwnerId == aLevelOwnerId);
	}

//This function validates number of dependency resource and their id's for correctness
void TestRM::CheckForDependencyInformation(TUint aClientId, TUint aResourceId, TUint aNumDependents, SResourceDependencyInfo* aDepResIdArray)
	{
	TUint numDepResources;

	//Get the number of dependent's for the resource
	r = lddChan.GetNumDependentsForResource(aClientId, aResourceId, numDepResources);
	if(r != KErrNone)
		test.Printf(_L("GetNumDependentsForResource returned with %d\n"), r);
	test(r == KErrNone);
	if(aNumDependents != numDepResources)
		test.Printf(_L("aNumDependents = %d, numDepResource = %d\n"), aNumDependents, numDepResources);
	test(aNumDependents == numDepResources);
	if(numDepResources == 0)
		return;
	//Get the dependent's id
	RBuf8 info;
	info.Create(aNumDependents * sizeof(SResourceDependencyInfo));
	r = lddChan.GetDependentsIdForResource(aClientId, aResourceId, (TAny*)&info, numDepResources);
	if(r != KErrNone)
		{
		test.Printf(_L("GetDependentsIdForResource returned with %d\n"), r);
		info.Close();
		}
	test(r == KErrNone);
	if(aNumDependents != numDepResources)
		{
		test.Printf(_L("aNumDependents = %d, numDepResource = %d\n"), aNumDependents, numDepResources);
		info.Close();
		}	
	test(aNumDependents == numDepResources);
	SResourceDependencyInfo* sResDepInfoPtr = (SResourceDependencyInfo*)info.Ptr();
	for(TUint count = 0; count < aNumDependents; count++, sResDepInfoPtr++)
		{
		if(sResDepInfoPtr->iResourceId != aDepResIdArray[count].iResourceId)
			{
			test.Printf(_L("Expected resourceId : %d, Returned ResourceId = %d\n"),sResDepInfoPtr->iResourceId, 
																		aDepResIdArray[count].iResourceId);
			info.Close();
			test(0);
			}
		if(sResDepInfoPtr->iDependencyPriority != aDepResIdArray[count].iDependencyPriority)
			{
			test.Printf(_L("Expected resource priority : %d, Returned resource priority = %d\n"),sResDepInfoPtr->iDependencyPriority, 
																		aDepResIdArray[count].iDependencyPriority);
			info.Close();
			test(0);
			}
		}
	info.Close();
	return;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0595
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests static resources with dependency.
//! @SYMTestActions		0	Register clients
//!						1	Check dependency information of each resource
//!						2	Register notifications
//!						3	Change Resource State of each static resource with dependency
//!						4	Get state of the resources and verify them for correctness
//!						5	Check notification count for correctness
//!						6	Deregister client level 	
//!						7	Deregister clients
//!
//! @SYMTestExpectedResults   0	API should return with KErrNone, panics otherwise.
//!							  1	API should return with KErrNone, panics otherwise.
//!							  2	API should return with KErrNone, panics otherwise.
//!							  3 API should return with KErrNone, panics otherwise.
//!							  4 API should return with KErrNone, panics otherwise.
//!							  5 API should return with KErrNone, panics otherwise.
//!							  6 API should return with KErrNone, panics otherwise.
//!							  7 API should return with KErrNone, panics otherwise.
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::TestStaticResourceWithDependency()
	{
	TUint count;
	RArray<SResourceDependencyInfo>depResArray;
	SResourceDependencyInfo sResDepInfo;
	TUint numClients; // The maximum no. of dependents in the dependency tree.
	
	
	//Register client 1.
	RmTest.RegisterClient();
	iCurrentClientId = -1;
	TInt state;
	TRequestStatus reqSet;
	
	NegativeTesting = EFalse;
	test.Next(_L("\nTesting static resource with dependency...."));

	//Check for resource dependency information of Resource D
	sResDepInfo.iResourceId = iStaticDependencyResources[1];
	sResDepInfo.iDependencyPriority = 1;
	depResArray.Append(sResDepInfo);
	sResDepInfo.iResourceId = iStaticDependencyResources[3]; 
	sResDepInfo.iDependencyPriority = 2;
	depResArray.Append(sResDepInfo);
	sResDepInfo.iResourceId = iStaticDependencyResources[2]; 
	sResDepInfo.iDependencyPriority = 3;
	depResArray.Append(sResDepInfo);
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, iStaticDependencyResources[0], 3, &depResArray[0]);
	
	//Check for resource dependency information of Resource E
	depResArray[0].iResourceId = iStaticDependencyResources[4]; 
	depResArray[0].iDependencyPriority = 1;
	depResArray[1].iResourceId = iStaticDependencyResources[5]; 
	depResArray[1].iDependencyPriority = 2;
	depResArray[2].iResourceId = iStaticDependencyResources[0]; 
	depResArray[2].iDependencyPriority = 3;
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, iStaticDependencyResources[3], 3, &depResArray[0]);
	
	//Check for resource dependency information of Resource C
	depResArray[0].iResourceId = iStaticDependencyResources[3]; 
	depResArray[0].iDependencyPriority = 1;
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, iStaticDependencyResources[4], 1, &depResArray[0]);
	
	//Check for resource dependency information of Resource G
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, iStaticDependencyResources[5], 1, &depResArray[0]);
	
	//Check for resource dependency information of Resource F
	depResArray[0].iResourceId = iStaticDependencyResources[0]; 
	depResArray[0].iDependencyPriority = 1;
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, iStaticDependencyResources[2], 1, &depResArray[0]);
	
	//Check for resource dependency information of Resource A
	RmTest.CheckForDependencyInformation(Clients[0].iClientId, iStaticDependencyResources[1], 1, &depResArray[0]);
	
	RmTest.GetNumResourcesInUseByClient(iCurrentClientId);
	RmTest.GetInfoOnResourcesInUseByClient(iCurrentClientId, iMaxStaticResources + iMaxStaticDependentResources);
	iCurrentClientId = 0;
	//Get resource state of all dependent resource and verify
	GetExtendedResStateAndVerify(iStaticDependencyResources[0], -50, -1);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -11, iStaticDependencyResources[0]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[2], 1, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[3], 13, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 73, iStaticDependencyResources[3]);

	r = lddChan.GetNumClientsUsingResource(Clients[0].iClientId, iStaticDependencyResources[3], numClients);
	test(r == KErrNone);
	test(numClients == 0);
	//Request notification 
	for(count = 0; count < iMaxStaticDependentResources; count++)
		{
		r = lddChan.RequestNotification(Clients[0].iClientId, iStaticDependencyResources[count]);
		test(r == KErrNone);
		}
	//Change state of resource A to -11 and verify
	r = lddChan.ChangeResourceStateSync(Clients[0].iClientId, iStaticDependencyResources[1], -11);
	test(r == KErrNone);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -11, Clients[0].iClientId);
	//Change state of resource A to -12 and verify
	state = -12;
	lddChan.ChangeResourceStateAsync(Clients[0].iClientId, iStaticDependencyResources[1], state, reqSet);
	User::WaitForRequest(reqSet);
	test(reqSet.Int() == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[1], 1, 0);
	test(r == KErrNone);
	//Register client2
	RmTest.RegisterClient();
	//Change state of resource D to -49 and verify
	state = -49;
	lddChan.ChangeResourceStateAsync(Clients[1].iClientId, iStaticDependencyResources[0], state, reqSet);
	User::WaitForRequest(reqSet);
	test(reqSet.Int() == KErrNone);
	//Check for notifications
	r = lddChan.CheckNotifications(iStaticDependencyResources[0], 1, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[1], 1, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[3], 1, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[5], 1, 0);
	test(r == KErrNone);
	//Get the state and verify for correctness
	GetExtendedResStateAndVerify(iStaticDependencyResources[0], -49, Clients[1].iClientId);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -13, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 1, iStaticDependencyResources[0]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[3], 16, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 71, iStaticDependencyResources[3]);
	//Change state of resource F to 1 and verify
	r = lddChan.ChangeResourceStateSync(Clients[1].iClientId, iStaticDependencyResources[2], 1);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[2], 0, 0);
	test(r == KErrNone);
	GetExtendedResStateAndVerify(iStaticDependencyResources[2], 1, Clients[1].iClientId);
	//Register client 3
	RmTest.RegisterClient();
	//Change state of resource E to 19 and verify
	r = lddChan.ChangeResourceStateSync(Clients[2].iClientId, iStaticDependencyResources[3], 19); 
	test(r == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[3], 1, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[5], 1, 0);
	test(r == KErrNone);
	GetExtendedResStateAndVerify(iStaticDependencyResources[0], -49, Clients[1].iClientId);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -13, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 1, Clients[1].iClientId);
	GetExtendedResStateAndVerify(iStaticDependencyResources[3], 19, Clients[2].iClientId);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 69, iStaticDependencyResources[3]);
	//Register client 4
	RmTest.RegisterClient();
	//Change state of resource C to 0 and verify
	r = lddChan.ChangeResourceStateSync(Clients[3].iClientId, iStaticDependencyResources[4], 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[4], 0, 0);
	test(r == KErrNone);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3]);
	//Change state of resource C to 1 and verify
	r = lddChan.ChangeResourceStateSync(Clients[2].iClientId, iStaticDependencyResources[4], 1);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[4], 0, 0);
	test(r == KErrNone);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3]);
	//Change state of resource G to 67 and verify
	r = lddChan.ChangeResourceStateSync(Clients[2].iClientId, iStaticDependencyResources[5], 67);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[5], 1, 0);
	test(r == KErrNone);
	GetExtendedResStateAndVerify(iStaticDependencyResources[0], -49, Clients[1].iClientId);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -13, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 1, Clients[1].iClientId);
	GetExtendedResStateAndVerify(iStaticDependencyResources[3], 19, Clients[2].iClientId);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 67, Clients[2].iClientId);
	//Change state of resource G to 67 and verify
	r = lddChan.ChangeResourceStateSync(Clients[3].iClientId, iStaticDependencyResources[5], 67);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[5], 0, 0);
	test(r == KErrNone);
	GetExtendedResStateAndVerify(iStaticDependencyResources[0], -49, Clients[1].iClientId);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -13, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 1, Clients[1].iClientId);
	GetExtendedResStateAndVerify(iStaticDependencyResources[3], 19, Clients[2].iClientId);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 67, Clients[2].iClientId);

	//Change the state of the resource E to 24
	state = 24;
	//Register client 5
	RmTest.RegisterClient();
	lddChan.ChangeResourceStateAsync(Clients[4].iClientId, iStaticDependencyResources[3], state, reqSet);
	User::WaitForRequest(reqSet);
	test(reqSet.Int() == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[3], 1, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[5], 1, 0);
	test(r == KErrNone);
	GetExtendedResStateAndVerify(iStaticDependencyResources[0], -49, Clients[1].iClientId);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -13, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 1, Clients[1].iClientId);
	GetExtendedResStateAndVerify(iStaticDependencyResources[3], 24, Clients[4].iClientId);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 65, iStaticDependencyResources[3]);

	//Change resource state of Resource D to -51
	r = lddChan.ChangeResourceStateSync(Clients[2].iClientId, iStaticDependencyResources[0], -51);
	test(r == KErrAccessDenied);
	GetExtendedResStateAndVerify(iStaticDependencyResources[0], -49, Clients[1].iClientId);

	//DeregisterClient 5
	r = lddChan.DeRegisterClient(Clients[4].iClientId);
	test(r == KErrNone);

	r = lddChan.CheckNotifications(iStaticDependencyResources[3], 1, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[5], 1, 0);
	test(r == KErrNone);
	
	GetExtendedResStateAndVerify(iStaticDependencyResources[3], 19, Clients[2].iClientId);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 63, iStaticDependencyResources[3]);

	//Change resource state of resource D to 50
	state = 50;
	lddChan.ChangeResourceStateAsync(Clients[1].iClientId, iStaticDependencyResources[0], state, reqSet);
	User::WaitForRequest(reqSet);
	test(reqSet.Int() == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[0], 1, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[1], 1, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[3], 1, 0);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(iStaticDependencyResources[5], 1, 0);
	test(r == KErrNone);

	GetExtendedResStateAndVerify(iStaticDependencyResources[0], 50, Clients[1].iClientId);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -14, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 1, Clients[1].iClientId);
	GetExtendedResStateAndVerify(iStaticDependencyResources[3], 22, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 61, iStaticDependencyResources[3]);

	//Change resource state of resource G to 61
	r = lddChan.ChangeResourceStateSync(Clients[3].iClientId, iStaticDependencyResources[5], 61);
	test(r == KErrNone);

	r = lddChan.CheckNotifications(iStaticDependencyResources[5], 0, 0);
	test(r == KErrNone);

	GetExtendedResStateAndVerify(iStaticDependencyResources[0], 50, Clients[1].iClientId);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -14, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 1, Clients[1].iClientId);
	GetExtendedResStateAndVerify(iStaticDependencyResources[3], 22, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 61, iStaticDependencyResources[3]);

	//Deregister client 4;
	r = lddChan.DeRegisterClient(Clients[3].iClientId);
	test(r == KErrNone);
	
	r = lddChan.CheckNotifications(iStaticDependencyResources[5], 0, 0);
	test(r == KErrNone);

	GetExtendedResStateAndVerify(iStaticDependencyResources[0], 50, Clients[1].iClientId);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -14, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 1, Clients[1].iClientId);
	GetExtendedResStateAndVerify(iStaticDependencyResources[3], 22, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 61, iStaticDependencyResources[3]);

	//Deregister client 3.
	r = lddChan.DeRegisterClient(Clients[2].iClientId);
	test(r == KErrNone);

	GetExtendedResStateAndVerify(iStaticDependencyResources[0], 50, Clients[1].iClientId);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -14, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 1, Clients[1].iClientId);
	GetExtendedResStateAndVerify(iStaticDependencyResources[3], 22, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 61, iStaticDependencyResources[3]);

	//Deregister client 0 from Resource A
	r = lddChan.DeRegisterClientLevelFromResource(Clients[0].iClientId, iStaticDependencyResources[1]);
	test(r == KErrNone);

	r = lddChan.CheckNotifications(iStaticDependencyResources[1], 0, 0);
	test(r == KErrNone);

	GetExtendedResStateAndVerify(iStaticDependencyResources[0], 50, Clients[1].iClientId);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -14, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 1, Clients[1].iClientId);
	GetExtendedResStateAndVerify(iStaticDependencyResources[3], 22, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 61, iStaticDependencyResources[3]);

	//Move Resource D to default
	r = lddChan.ChangeResourceStateSync(Clients[1].iClientId, iStaticDependencyResources[0], -100);
	test(r == KErrPermissionDenied);

	//Deregister client 1 from Resource F
	r = lddChan.DeRegisterClientLevelFromResource(Clients[1].iClientId, iStaticDependencyResources[2]);
	test(r == KErrNone);

	r = lddChan.CheckNotifications(iStaticDependencyResources[1], 0, 0);
	test(r == KErrNone);

	GetExtendedResStateAndVerify(iStaticDependencyResources[0], 50, Clients[1].iClientId);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -14, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 1, iStaticDependencyResources[0]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[3], 22, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 61, iStaticDependencyResources[3]);

	//Deregister client 2
	r = lddChan.DeRegisterClient(Clients[1].iClientId);
	test(r == KErrNone);

	GetExtendedResStateAndVerify(iStaticDependencyResources[0], -100, -1);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -10, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 0, iStaticDependencyResources[0]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[3], 10, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 1, iStaticDependencyResources[3]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 75, iStaticDependencyResources[3]);
	//Deregister client 1
	r = lddChan.DeRegisterClient(Clients[0].iClientId);
	test(r == KErrNone);
	Clients.Close(); //Close the array and release memory
	
	//Test parallel execution of RC and Dependency resource DFC's
	//Register client 1
	RmTest.RegisterClient();
	//Register client 2
	RmTest.RegisterClient();

	state = 50;
	/* CheckParallelExecutionForResChageStateWithDependency */
	lddChan.CheckParallelExecutionForChangeResState(Clients[1].iClientId,
								iStaticDependencyResources[0],state,5,0,reqSet);

	User::WaitForRequest(reqSet);
	test(reqSet.Int() == KErrNone);
	
	GetExtendedResStateAndVerify(iStaticDependencyResources[0], 50, Clients[1].iClientId);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -11, iStaticDependencyResources[0]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[2], 1, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[3], 13, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 0, iStaticDependencyResources[3]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 73, iStaticDependencyResources[3]);

	TInt owner;
	TBool cached = ETrue;
	r = lddChan.GetResourceStateSync(Clients[1].iClientId, 4, cached, state, owner);
	test(r == KErrNone);
	test(state == 75);
	test(owner == -1);

	r = lddChan.DeRegisterClient(Clients[1].iClientId);
	test(r == KErrNone);

	GetExtendedResStateAndVerify(iStaticDependencyResources[0], -100, -1);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[1], -10, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[2], 0, iStaticDependencyResources[0]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[3], 10, iStaticDependencyResources[0]);
	GetExtendedResStateAsyncAndVerify(iStaticDependencyResources[4], 1, iStaticDependencyResources[3]);
	GetExtendedResStateAndVerify(iStaticDependencyResources[5], 75, iStaticDependencyResources[3]);

	r = lddChan.GetResourceStateSync(Clients[0].iClientId, 4, cached, state, owner);
	test(r == KErrNone);
	test(state == 75);
	test(owner == -1);

	r = lddChan.DeRegisterClient(Clients[0].iClientId);
	test(r == KErrNone);

	Clients.Close(); //Close the array and release memory
	depResArray.Close(); //Close the array and release memory
	}
#endif
	
//This function validates each of the resource manager API's
void TestRM::APIValidationTest()
	{
	test.Next(_L("\nStarting API validation Test...."));
	RmTest.RegisterClient();
 	r = lddChan.GetResourceControllerVersion(Clients[0].iClientId, iTestingExtendedVersion);
	test(r == KErrNone);
	if(!iTestingExtendedVersion)
		test.Printf(_L("Testing Basic Version only...."));
	else
		test.Printf(_L("Testing basic & extended version...."));
   RmTest.ValidateClient(5, EOwnerProcess);
	iCurrentClientId = -1;
	RmTest.GetNumResourcesInUseByClient(iCurrentClientId);
	RmTest.GetInfoOnResourcesInUseByClient(iCurrentClientId, iMaxStaticResources);
	iCurrentClientId = 0;
	NegativeTesting = ETrue;
	if(Resources[0].iName.Compare(*(const TDesC8*)&SpecialResName))
		{
		test.Printf(_L("Test runs only on simulated PSL\n"));
		RmTest.DeRegisterClient(Clients[0].iClientId);
		return;
		}	
 	TBuf8<32> PowerController = _L8("PowerController");
	r = lddChan.GetClientId(Clients[0].iClientId, (TDesC8&)PowerController, iPowerControllerId);
	test(r == KErrNone);

	RBuf8 info;
	TUint c;
	r = info.Create((iMaxStaticResources) * sizeof(SIdleResourceInfo));
	test(r == KErrNone);
	SIdleResourceInfo* pI = (SIdleResourceInfo*)info.Ptr();
	for(c = 0; c < iMaxStaticResources; c++)
		{
		pI->iResourceId = Resources[c].iResourceId;
		pI++;
		}
	pI = (SIdleResourceInfo*)info.Ptr();

	r = lddChan.RegisterForIdleResourcesInfo(iPowerControllerId, iMaxStaticResources, (TAny*)info.Ptr());

	test(r == KErrNone);
	RmTest.GetClientName(iCurrentClientId);
	RmTest.GetClientId(iCurrentClientId);
	RmTest.GetResourceId(2);
	RmTest.GetResourceInfo(19);
	RmTest.GetNumResourcesInUseByClient(iCurrentClientId);
	RmTest.GetInfoOnResourcesInUseByClient(iCurrentClientId, 3);
	RmTest.GetNumClientsUsingResource(iCurrentClientId, (TUint)-1);
	RmTest.GetNumClientsUsingResource(iCurrentClientId, 10);
	RmTest.GetInfoOnClientsUsingResource((TUint)-1, 4);
	RmTest.GetInfoOnClientsUsingResource(5, 3);
	
	for(c = 0; c < iMaxStaticResources; c++)
		{
		if(Resources[c].iSense == ECustom)
			continue;
		RmTest.GetResourceStateAsync(c, ETrue);
		}
	User::After(2000000);  //Add delay to make sure that the asynchronous request is processed in controller thread

	for(c = 0; c < iMaxStaticResources; c++)
		{
		if(Resources[c].iSense == ECustom)
			continue;
		iCurrentClientId = c;
		RmTest.RegisterClient();
		RmTest.AllocReserve(c);
		RmTest.GetResourceStateAsync(c);
		RmTest.RequestNotification(c);
		RmTest.RequestNotificationCon(c);
		}

	for(c=0; c< iMaxStaticResources; c++)
		{
		if(Resources[c].iSense == ECustom)
			continue;
		iCurrentClientId = c;
		RmTest.ChangeResourceStateAsync(c);
		RmTest.GetResourceStateAsync(c);
		RmTest.GetResourceStateSync(c);
		RmTest.ChangeResourceStateSync(c);
		}
	for(c = 0; c < iMaxStaticResources; c++)
		{
		if(Resources[c].iSense == ECustom)
			continue;
		iCurrentClientId = c;
		RmTest.GetClientName(c);
		RmTest.GetClientId(c);
		RmTest.GetResourceId(c);
		RmTest.GetResourceInfo(c);
		RmTest.GetNumResourcesInUseByClient(c);
		RmTest.GetInfoOnResourcesInUseByClient(c, Clients[c].iNumResources);
		RmTest.GetNumClientsUsingResource(c, c);
		RmTest.GetInfoOnClientsUsingResource(c, Resources[c].iNumClients);
		RmTest.CancelNotification(c, ETrue);
		RmTest.CancelNotification(c, EFalse);
		}

	TInt clientCount = Clients.Count();
	for(c = clientCount-1; ((TInt)c) >=0; c--)
	{ 
		test.Printf(_L("DeRegister ClientId %d\n"), Clients[c].iClientId);
		RmTest.DeRegisterClient(c);
		}
	Clients.Close();
	//Find any shared binary resource
	for(c = 0; c < iMaxStaticResources; c++)
		{
		if(Resources[c].iSense == ECustom)
			continue;
		if((Resources[c].iUsage == EShared) && (Resources[c].iSense == ENegative))
			{
			if(Resources[c].iType == 0x0) //Binary Resource
				RmTest.SharedBinaryNegativeResourceTesting(c);
			else 
				RmTest.SharedMultilevelNegativeResourceTesting(c);
			}
		else if((Resources[c].iUsage == EShared) && (Resources[c].iSense == EPositive))
			{
			if(Resources[c].iType == 0x0) //Binary Resource
				RmTest.SharedBinaryPositiveResourceTesting(c);
			else
				RmTest.SharedMultilevelPositiveResourceTesting(c);
			}
		}

	RmTest.CustomResourceTesting(CUSTOM_RESOURCE_NUMBER);
  
	//Testing of Deregistration of client level for binary resource
	RmTest.RegisterClient();
	for(c = 0; c < iMaxStaticResources; c++)
		{
		if(Resources[c].iSense == ECustom)
			continue;
		RmTest.DeRegisterClientLevelFromResource(-1, c);
		}
	for(c = 0; c < iMaxStaticResources; c++)
		{
		iCurrentClientId = 0;
		RmTest.ChangeResourceStateSync(c);
		RmTest.DeRegisterClientLevelFromResource(0, c);
		}
	RmTest.RegisterClient();
	for(c = 0; c < iMaxStaticResources; c++) //Test valid only for shared resources.
		{
		if((Resources[c].iSense == ECustom) || (Resources[c].iUsage == ESingle))
			continue;
		iCurrentClientId = 0;
		RmTest.ChangeResourceStateSync(c);
		iCurrentClientId  = 1;
		RmTest.ChangeResourceStateSync(c);
		if(Resources[c].iCurrentClient == 0)
			{
			RmTest.DeRegisterClientLevelFromResource(0, c);
			RmTest.DeRegisterClientLevelFromResource(1, c);
			}
		else
			{
			RmTest.DeRegisterClientLevelFromResource(1, c);
			RmTest.DeRegisterClientLevelFromResource(0, c);
			}
		}
	//Testing of Deregistration of client level for shared resource
	for(c = 0; c < iMaxStaticResources; c++)
		{
		if((Resources[c].iSense == ECustom) || (!Resources[c].iUsage))
			continue;
		RmTest.DeRegisterClientLevelFromResource(-1, c);
		}
	
	RmTest.DeRegisterClient(1);
	RmTest.DeRegisterClient(0);
	info.Create(15 * sizeof(SIdleResourceInfo));
	r = lddChan.GetIdleResourcesInfo(15, (TAny*)(TDes8*)&info);
	test(r == KErrNone);
	pI = (SIdleResourceInfo*)info.Ptr();
	for(c = 0; c< 15; c++)
	{
	   test(Resources[c].iCurrentClient == pI->iLevelOwnerId);
	   test(Resources[c].iCurrentLevel == pI->iCurrentLevel);
	   test(Resources[c].iResourceId == pI->iResourceId);
	   pI++;
	}
	info.Close();
	Clients.Close();
#ifdef PRM_ENABLE_EXTENDED_VERSION
	if(iTestingExtendedVersion)
		{
		TestStaticResourceWithDependency();
		TestDynamicResource();
		TestDynamicResourceDependency();
		}
#endif
	Clients.Close();
	return;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0592
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests deregistration of client level functionality.
//! @SYMTestActions		0	Register client
//!						1	Change Resource State
//!						2	Deregister client level
//!						3	Deregister client
//!
//! @SYMTestExpectedResults   0	API should return with KErrNone, panics otherwise.
//!							  1	API should return with KErrNone, panics otherwise.
//!							  2	API should return with KErrNone, panics otherwise.
//!							  3 API should return with KErrNone, panics otherwise.
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::DeRegisterClientLevelFromResource(TInt aClientId, TUint aResId)
	{
	TInt state;
	TInt newState;
	TInt levelOwnerId;
	r = lddChan.GetResourceStateSync(Clients[0].iClientId, Resources[aResId].iResourceId, ETrue, newState, levelOwnerId);
	test(r == KErrNone);
	if((levelOwnerId != -1) && (levelOwnerId != (TInt)Clients[aClientId].iClientId))
		{
		test.Printf(_L("Client Id does not match so not testing Deregistration of client level\n"));
		return;
		}
	if(Resources[aResId].iUsage == ESingle) //Single user resource
		{
		if(levelOwnerId == -1) 
			{
			TUint ClientId;
			ClientName[6] = (TUint8)('0' + iMaxClientId+1);
			r = lddChan.RegisterClient(ClientId, (const TDesC*)&ClientName);
			test(r == KErrNone);
			newState = Resources[aResId].iMaxLevel;
			r = lddChan.ChangeResourceStateSync(ClientId, Resources[aResId].iResourceId, newState);
			test(r == KErrNone);
			r = lddChan.GetResourceStateSync(ClientId, Resources[aResId].iResourceId, ETrue, state, levelOwnerId);
			test(r == KErrNone);
			test(state == newState);
			test(levelOwnerId == (TInt)ClientId);
			r = lddChan.DeRegisterClientLevelFromResource(ClientId, Resources[aResId].iResourceId);
			test(r == KErrNone);
			r = lddChan.GetResourceStateSync(ClientId, Resources[aResId].iResourceId, ETrue, state, levelOwnerId);
			test(r == KErrNone);
			test(levelOwnerId == -1);
			r = lddChan.DeRegisterClient(ClientId);
			test(r == KErrNone);
			return;
			}
		r = lddChan.DeRegisterClientLevelFromResource(Clients[aClientId].iClientId, Resources[aResId].iResourceId);
		test(r == KErrNone);
		r = lddChan.GetResourceStateSync(Clients[aClientId].iClientId, Resources[aResId].iResourceId, ETrue, state, levelOwnerId);
		test(r == KErrNone);
		test(levelOwnerId == -1);
		//Update the local
		Resources[aResId].iCurrentClient = -1;
		Resources[aResId].iCurrentLevel = state;
		Resources[aResId].iNumClients = 0;
		delete Resources[aResId].iLevel;
		Resources[aResId].iLevel = NULL;
		return;
		}
	//Handle for Shared resources
	if(levelOwnerId == -1)
		{
		TUint ClientId[2];
		ClientName[6] = (TUint8)('0' + iMaxClientId+1);
		r = lddChan.RegisterClient(ClientId[0], (const TDesC*)&ClientName);
		test(r == KErrNone);
		ClientName[6] = (TUint8)('0' + iMaxClientId+2);
		r = lddChan.RegisterClient(ClientId[1], (const TDesC*)&ClientName);
		test(r == KErrNone);
		newState = Resources[aResId].iMinLevel;
		r = lddChan.ChangeResourceStateSync(ClientId[0], Resources[aResId].iResourceId, newState);
		test(r == KErrNone);
		r = lddChan.GetResourceStateSync(ClientId[0], Resources[aResId].iResourceId, ETrue, state, levelOwnerId);
		test(r == KErrNone);
		test(levelOwnerId == (TInt)ClientId[0]);
		r = lddChan.ChangeResourceStateSync(ClientId[1], Resources[aResId].iResourceId, newState);
		test(r == KErrNone);
		r = lddChan.DeRegisterClientLevelFromResource(ClientId[0], Resources[aResId].iResourceId);
		test(r == KErrNone);
		r = lddChan.GetResourceStateSync(ClientId[0], Resources[aResId].iResourceId, ETrue, state, levelOwnerId);
		test(r == KErrNone);
		test(levelOwnerId == (TInt)ClientId[1]);
		r = lddChan.DeRegisterClientLevelFromResource(ClientId[1], Resources[aResId].iResourceId);
		test(r == KErrNone);
		r = lddChan.GetResourceStateSync(ClientId[0], Resources[aResId].iResourceId, ETrue, state, levelOwnerId);
		test(r == KErrNone);
		test(levelOwnerId == -1);
		r = lddChan.DeRegisterClient(ClientId[0]);
		test(r == KErrNone);
		r = lddChan.DeRegisterClient(ClientId[1]);
		test(r == KErrNone);
		return;
		}
	r = lddChan.DeRegisterClientLevelFromResource(Clients[aClientId].iClientId, Resources[aResId].iResourceId);
	test(r == KErrNone);
	r = lddChan.GetResourceStateSync(Clients[aClientId].iClientId, Resources[aResId].iResourceId, ETrue, state, levelOwnerId);
	test(r == KErrNone);
	test(levelOwnerId != (TInt)Clients[aClientId].iClientId);
	if(Resources[aResId].iNumClients == 1)
		{
		Resources[aResId].iNumClients--;
		Resources[aResId].iCurrentClient = -1;
		r = lddChan.GetResourceStateSync(Clients[aClientId].iClientId, Resources[aResId].iResourceId, ETrue, state, levelOwnerId);
		test(r == KErrNone);
		Resources[aResId].iCurrentLevel = state;
		delete Resources[aResId].iLevel;
		Resources[aResId].iLevel = NULL;
		}
	else
		{
		Resources[aResId].iNumClients--;
		SPowerResourceClientLevel *pCL = NULL;
		TInt level = KMinTInt;
		TInt clientId = 0;
		for(SPowerResourceClientLevel* pL = Resources[aResId].iLevel; pL != NULL; pL = pL->iNextInList)
			{
			if(pL->iClientId == Clients[aClientId].iClientId)
				{
				pCL = pL;
				continue;
				}
			if(level == KMinTInt)
				{
				level = pL->iLevel;
				clientId = pL->iClientId;
				continue;
				}
			if(((Resources[aResId].iSense == EPositive) && (pL->iLevel > level)) || ((Resources[aResId].iSense == ENegative) && (pL->iLevel < level)))
				{
				level = pL->iLevel;
				clientId = pL->iClientId;
				}
			}
		delete pCL;
		Resources[aResId].iCurrentClient = clientId;
		Resources[aResId].iCurrentLevel = level;
		}
	return;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0593
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests changing resource state of shared positive resource.
//! @SYMTestActions		0	Register client1
//!						1	Register client2
//!						2	Register client3
//!						3	Register client4
//!						4	Client1 change resource state.
//!						5	Client2 change resource state.
//!						6	Client3 change resource state.
//!						7	Client4 change resource state.
//!						8	Client1 change resource state.
//!						9	Client2 change resource state.
//!						10	Deregister client2
//!						11	Client3 change resource state.
//!						12	Deregister client1
//!						13	Deregister client3
//!						14	Deregister client4
//!
//! @SYMTestExpectedResults   0	Client registered
//!							  1	Client registered
//!							  2 Client registered
//!							  3	Client registered
//!							  4 Resource state changed
//!							  5	Resource state changed
//!							  6	Resource state changed
//!							  7 Resource state changed
//!							  8 Resource state changed
//!							  9 Resource state changed
//!							 10 Client2 deregistered
//!							 11 Resource state changed
//!							 12 Client1 deregistered
//!							 13 Client3 deregistered
//!							 14 Client4	deregistered	
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::SharedBinaryPositiveResourceTesting(TUint aResId)
	{
	TInt newState, levelOwnerId;
	TRequestStatus req;
	TUint ClientId[5];

	ClientName[6] = (TUint8)('0' + iMaxClientId+1);
	r = lddChan.RegisterClient(ClientId[0], (const TDesC*)&ClientName);
	test(r == KErrNone);

	r = lddChan.GetResourceStateSync(ClientId[0], Resources[aResId].iResourceId, ETrue, newState, levelOwnerId);
	test(r == KErrNone);
	if(levelOwnerId != -1)
		{
		test.Printf(_L("Not testing the shared resource as some other client is currently holding the resource\n"));
		r = lddChan.DeRegisterClient(ClientId[0]);
		test(r == KErrNone);
		return;
		}

	ClientName[6] = (TUint8)('0' + iMaxClientId +2);
	r = lddChan.RegisterClient(ClientId[1], (const TDesC*)&ClientName);
	test(r == KErrNone);
	ClientName[6] = (TUint8)('0' + iMaxClientId +3);
	r = lddChan.RegisterClient(ClientId[2], (const TDesC*)&ClientName);
	test(r == KErrNone);
	ClientName[6] = (TUint8)('0' + iMaxClientId +4);
	r = lddChan.RegisterClient(ClientId[3], (const TDesC*)&ClientName);
	test(r == KErrNone);
	ClientName[6] = (TUint8)('0' + iMaxClientId +5);
	r = lddChan.RegisterClient(ClientId[4], (const TDesC*)&ClientName);
	test(r == KErrNone);

	newState = 1;
	r = lddChan.ChangeResourceStateSync(ClientId[0], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 0, 0);
	test(r == KErrNone);
	r = lddChan.RequestNotification(ClientId[1], Resources[aResId].iResourceId);
	test(r == KErrNone);
	r = lddChan.RequestNotification(ClientId[2], Resources[aResId].iResourceId);
	test(r == KErrNone);
	r = lddChan.RequestNotification(ClientId[3], Resources[aResId].iResourceId, 1, ETrue);
	test(r == KErrNone);
	r = lddChan.RequestNotification(ClientId[2], Resources[aResId].iResourceId, 0, EFalse);
	test(r == KErrNone);
	newState = !newState;	   //State 0
	r = lddChan.ChangeResourceStateSync(ClientId[0], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 2, 1);
	test(r == KErrNone);
	newState = !newState;	//State 1
	r = lddChan.ChangeResourceStateSync(ClientId[1], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 2, 1); 
	test(r == KErrNone);
	lddChan.ChangeResourceStateAsync(ClientId[2], Resources[aResId].iResourceId, newState, req);
	User::WaitForRequest(req); //State 1
	test(req.Int() == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 0, 0);
	test(r == KErrNone);
	newState = !newState;			   //State 0
	r = lddChan.ChangeResourceStateSync(ClientId[3], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 0, 0);
	test(r == KErrNone);
	newState = !newState;	//state 1
	r = lddChan.ChangeResourceStateSync(ClientId[0], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 0, 0);
	newState = !newState;   //state 0
	r = lddChan.ChangeResourceStateSync(ClientId[1], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 0,0);
	test(r == KErrNone);
	r = lddChan.DeRegisterClient(ClientId[1]);
	test(r == KErrNone);
	newState = 0;
	r = lddChan.ChangeResourceStateSync(ClientId[2], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 0,0);
	test(r == KErrNone);
	r = lddChan.ChangeResourceStateSync(ClientId[0], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 1,1); 
	test(r == KErrNone);
	r = lddChan.CancelNotification(ClientId[2], Resources[aResId].iResourceId, EFalse);
	test(r == KErrCancel);
	r = lddChan.CancelNotification(ClientId[3], Resources[aResId].iResourceId, ETrue);
	test(r == KErrCancel);
	newState = 1;
	r = lddChan.ChangeResourceStateSync(ClientId[0], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 0, 0);
	test(r == KErrNone);
	r = lddChan.CancelNotification(ClientId[2], Resources[aResId].iResourceId, ETrue);
	test(r == KErrCancel);
	newState = 1;
	r = lddChan.ChangeResourceStateSync(ClientId[0], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 0, 0);
	test(r == KErrNone);
	r = lddChan.DeRegisterClient(ClientId[0]);
	test(r == KErrNone);
	r = lddChan.DeRegisterClient(ClientId[2]);
	test(r == KErrNone);
	r = lddChan.DeRegisterClient(ClientId[3]);
	test(r == KErrNone);
	r = lddChan.GetResourceStateSync(ClientId[4], Resources[aResId].iResourceId, ETrue, newState, levelOwnerId);
	test(r == KErrNone);
	test(levelOwnerId == -1);
	r = lddChan.DeRegisterClient(ClientId[4]);
	test(r == KErrNone);
	return;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0594
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests changing resource state of shared negative resource.
//! @SYMTestActions		0	Register client1
//!						1	Register client2
//!						2	Register client3
//!						3	Register client4
//!						4	Client1 change resource state.
//!						5	Client2 change resource state.
//!						6	Client3 change resource state.
//!						7	Client4 change resource state.
//!						8	Client1 change resource state.
//!						9	Client2 change resource state.
//!						10	Deregister client2
//!						11	Client3 change resource state.
//!						12	Deregister client1
//!						13	Deregister client3
//!						14	Deregister client4
//!
//! @SYMTestExpectedResults   0	Client registered
//!							  1	Client registered
//!							  2 Client registered
//!							  3	Client registered
//!							  4 Resource state changed
//!							  5	Resource state changed
//!							  6	Resource state changed
//!							  7 Resource state changed
//!							  8 Resource state changed
//!							  9 Resource state changed
//!							 10 Client2 deregistered
//!							 11 Resource state changed
//!							 12 Client1 deregistered
//!							 13 Client3 deregistered
//!							 14 Client4	deregistered	
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::SharedBinaryNegativeResourceTesting(TUint aResId)
	{
	TInt newState;
	TInt levelOwnerId;
	TRequestStatus req;
	TUint ClientId[5];

	ClientName[6] = (TUint8)('0' + iMaxClientId+1);
	r = lddChan.RegisterClient(ClientId[0], (const TDesC*)&ClientName);
	test(r == KErrNone);

	r = lddChan.GetResourceStateSync(ClientId[0], Resources[aResId].iResourceId, ETrue, newState, levelOwnerId);
	test(r == KErrNone);
	if(levelOwnerId != -1)
		{
		test.Printf(_L("Not testing the shared resource as some other client is currently holding the resource\n"));
		r = lddChan.DeRegisterClient(ClientId[0]);
		test(r == KErrNone);
		return;
		}

	ClientName[6] = (TUint8)('0' + iMaxClientId +2);
	r = lddChan.RegisterClient(ClientId[1], (const TDesC*)&ClientName);
	test(r == KErrNone);
	ClientName[6] = (TUint8)('0' + iMaxClientId +3);
	r = lddChan.RegisterClient(ClientId[2], (const TDesC*)&ClientName);
	test(r == KErrNone);
	ClientName[6] = (TUint8)('0' + iMaxClientId +4);
	r = lddChan.RegisterClient(ClientId[3], (const TDesC*)&ClientName);
	test(r == KErrNone);
	ClientName[6] = (TUint8)('0' + iMaxClientId +5);
	r = lddChan.RegisterClient(ClientId[4], (const TDesC*)&ClientName);
	test(r == KErrNone);

	newState = 0;
	r = lddChan.ChangeResourceStateSync(ClientId[0], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 0, 0);
	test(r == KErrNone);
	r = lddChan.RequestNotification(ClientId[1], Resources[aResId].iResourceId);
	test(r == KErrNone);
	r = lddChan.RequestNotification(ClientId[2], Resources[aResId].iResourceId);
	test(r == KErrNone);
	r = lddChan.RequestNotification(ClientId[3], Resources[aResId].iResourceId, 1, ETrue);
	test(r == KErrNone);
	r = lddChan.RequestNotification(ClientId[2], Resources[aResId].iResourceId, 0, EFalse);
	test(r == KErrNone);
	newState = !newState;	   //State 1
	r = lddChan.ChangeResourceStateSync(ClientId[0], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 2, 1);
	test(r == KErrNone);
	newState = !newState;	//State 0
	r = lddChan.ChangeResourceStateSync(ClientId[1], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 2, 1); 
	test(r == KErrNone);
	lddChan.ChangeResourceStateAsync(ClientId[2], Resources[aResId].iResourceId, newState, req);
	User::WaitForRequest(req); //State 0
	test(req.Int() == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 0, 0);
	test(r == KErrNone);
	newState = !newState;			   //State 1
	r = lddChan.ChangeResourceStateSync(ClientId[3], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 0, 0);
	test(r == KErrNone);
	newState = !newState;	//state 0
	r = lddChan.ChangeResourceStateSync(ClientId[0], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 0, 0);
	newState = !newState;   //state 1
	r = lddChan.ChangeResourceStateSync(ClientId[1], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 0,0);
	test(r == KErrNone);
	r = lddChan.DeRegisterClient(ClientId[1]);
	test(r == KErrNone);
	newState = 1;
	r = lddChan.ChangeResourceStateSync(ClientId[2], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 0,0);
	test(r == KErrNone);
	r = lddChan.ChangeResourceStateSync(ClientId[0], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 1,1); 
	test(r == KErrNone);
	r = lddChan.CancelNotification(ClientId[2], Resources[aResId].iResourceId, EFalse);
	test(r == KErrCancel);
	r = lddChan.CancelNotification(ClientId[3], Resources[aResId].iResourceId, ETrue);
	test(r == KErrCancel);
	newState = 1;
	r = lddChan.ChangeResourceStateSync(ClientId[0], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 0, 0);
	test(r == KErrNone);
	r = lddChan.CancelNotification(ClientId[2], Resources[aResId].iResourceId, ETrue);
	test(r == KErrCancel);
	newState = 1;
	r = lddChan.ChangeResourceStateSync(ClientId[0], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 0, 0);
	test(r == KErrNone);
	r = lddChan.DeRegisterClient(ClientId[0]);
	test(r == KErrNone);
	r = lddChan.DeRegisterClient(ClientId[2]);
	test(r == KErrNone);
	r = lddChan.DeRegisterClient(ClientId[3]);
	test(r == KErrNone);
	r = lddChan.GetResourceStateSync(ClientId[4], Resources[aResId].iResourceId, ETrue, newState, levelOwnerId);
	test(r == KErrNone);
	test(levelOwnerId == -1);
	r = lddChan.DeRegisterClient(ClientId[4]);
	test(r == KErrNone);
	return;
	}

//Test cases to test the shared multilevel negative resources
void TestRM::SharedMultilevelNegativeResourceTesting(TUint aResId)
	{
	TInt newState;
	TInt levelOwnerId;
	TRequestStatus req;
	TUint ClientId[2];

	//Register 1st client
	ClientName[6] = (TUint8)('0' + iMaxClientId+1);
	r = lddChan.RegisterClient(ClientId[0], (const TDesC*)&ClientName);
	test(r == KErrNone);

	r = lddChan.GetResourceStateSync(ClientId[0], Resources[aResId].iResourceId, ETrue, newState, levelOwnerId);
	test(r == KErrNone);
	if(levelOwnerId != -1)
		{
		test.Printf(_L("Not testing the shared resource as some other client is currently holding the resource\n"));
		r = lddChan.DeRegisterClient(ClientId[0]);
		test(r == KErrNone);
		return;
		}

	test.Printf(_L("Testing %d Shared Multilevel Negative Resource\n"), Resources[aResId].iResourceId);

	//Register 2nd client
	ClientName[6] = (TUint8)('0' + iMaxClientId+2);
	r = lddChan.RegisterClient(ClientId[1], (const TDesC*)&ClientName);
	test(r == KErrNone);

	//Change the resource and ClientId[0] becomes the owner of the resource
	newState = Resources[aResId].iMaxLevel + 10;
	
	r = lddChan.ChangeResourceStateSync(ClientId[0], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);

	TInt state;
	r = lddChan.GetResourceStateSync(ClientId[0], Resources[aResId].iResourceId, ETrue, state, levelOwnerId);
	test(r == KErrNone);
	test(state == newState);
	test(levelOwnerId = (TInt)ClientId[0]);

	//Second client(clientId[1]) trying to change the resource, but still 
	newState = state +5;
	lddChan.ChangeResourceStateAsync(ClientId[1], Resources[aResId].iResourceId, newState, req);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);

	lddChan.GetResourceStateAsync(ClientId[0], Resources[aResId].iResourceId, EFalse, req, newState, levelOwnerId);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);
	test(state = newState);
	test(levelOwnerId == (TInt)ClientId[0]);

	newState = state + 10;
	r = lddChan.ChangeResourceStateSync(ClientId[0], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	
	r = lddChan.GetResourceStateSync(ClientId[0], Resources[aResId].iResourceId, EFalse, state, levelOwnerId);
	test(r == KErrNone);
	newState = Resources[aResId].iMaxLevel + 15;
	test(state == newState);
	test(levelOwnerId == (TInt)ClientId[1]);

	r = lddChan.DeRegisterClientLevelFromResource(ClientId[1], Resources[aResId].iResourceId);
	test(r == KErrNone);

	state = Resources[aResId].iMaxLevel + 20;
	lddChan.GetResourceStateAsync(ClientId[1], Resources[aResId].iResourceId, ETrue, req, newState, levelOwnerId);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);
	test(state == newState);
	test(levelOwnerId == (TInt)ClientId[0]);

	newState = Resources[aResId].iMaxLevel + 10;
	lddChan.ChangeResourceStateAsync(ClientId[1], Resources[aResId].iResourceId, newState, req);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);


	lddChan.GetResourceStateAsync(ClientId[0], Resources[aResId].iResourceId, EFalse, req, state, levelOwnerId);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);
	test(state == newState);
	test(levelOwnerId == (TInt)ClientId[1]);
	
	r = lddChan.DeRegisterClient(ClientId[1]);
	test(r == KErrNone);
     
	r = lddChan.DeRegisterClientLevelFromResource(ClientId[0], Resources[aResId].iResourceId);
	test(r == KErrNone);

	r = lddChan.GetResourceStateSync(ClientId[0], Resources[aResId].iResourceId, EFalse, state, levelOwnerId);
	test(r == KErrNone);
	test(state == Resources[aResId].iDefaultLevel);
	test(levelOwnerId == -1);

	r = lddChan.DeRegisterClient(ClientId[0]);
	test(r == KErrNone);

	return;
	}

//Test cases to test the shared multilevel positive resources
void TestRM::SharedMultilevelPositiveResourceTesting(TUint aResId)
	{
	TInt newState;
	TInt levelOwnerId;
	TRequestStatus req;
	TUint ClientId[2];

	//Register 1st client
	ClientName[6] = (TUint8)('0' + iMaxClientId+1);
	r = lddChan.RegisterClient(ClientId[0], (const TDesC*)&ClientName);
	test(r == KErrNone);

	r = lddChan.GetResourceStateSync(ClientId[0], Resources[aResId].iResourceId, ETrue, newState, levelOwnerId);
	test(r == KErrNone);
	if(levelOwnerId != -1)
		{
		test.Printf(_L("Not testing the shared resource as some other client is currently holding the resource\n"));
		r = lddChan.DeRegisterClient(ClientId[0]);
		test(r == KErrNone);
		return;
		}

	test.Printf(_L("Testing %d Shared Multilevel positive Resource\n"), Resources[aResId].iResourceId);

	//Register 2nd client
	ClientName[6] = (TUint8)('0' + iMaxClientId+2);
	r = lddChan.RegisterClient(ClientId[1], (const TDesC*)&ClientName);
	test(r == KErrNone);

	//Change the resource and ClientId[0] becomes the owner of the resource
	newState = Resources[aResId].iMinLevel + 20;
	
	r = lddChan.ChangeResourceStateSync(ClientId[0], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);

	TInt state;
	r = lddChan.GetResourceStateSync(ClientId[0], Resources[aResId].iResourceId, ETrue, state, levelOwnerId);
	test(r == KErrNone);
	test(state == newState);
	test(levelOwnerId = (TInt)ClientId[0]);

	//Second client(clientId[1]) trying to change the resource, but still 
	newState = Resources[aResId].iMinLevel +10;
	lddChan.ChangeResourceStateAsync(ClientId[1], Resources[aResId].iResourceId, newState, req);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);

	lddChan.GetResourceStateAsync(ClientId[0], Resources[aResId].iResourceId, EFalse, req, newState, levelOwnerId);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);
	test(state = newState);
	test(levelOwnerId == (TInt)ClientId[0]);

	newState = Resources[aResId].iMinLevel + 5;
	r = lddChan.ChangeResourceStateSync(ClientId[0], Resources[aResId].iResourceId, newState);
	test(r == KErrNone);
	
	r = lddChan.GetResourceStateSync(ClientId[0], Resources[aResId].iResourceId, EFalse, state, levelOwnerId);
	test(r == KErrNone);
	test(state == Resources[aResId].iMinLevel+10);
	test(levelOwnerId == (TInt)ClientId[1]);

	r = lddChan.DeRegisterClientLevelFromResource(ClientId[1], Resources[aResId].iResourceId);
	test(r == KErrNone);

	newState = Resources[aResId].iMinLevel + 5;
	lddChan.GetResourceStateAsync(ClientId[1], Resources[aResId].iResourceId, ETrue, req, state, levelOwnerId);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);
	test(state == newState);
	test(levelOwnerId == (TInt)ClientId[0]);

	newState = Resources[aResId].iMinLevel + 10;
	lddChan.ChangeResourceStateAsync(ClientId[1], Resources[aResId].iResourceId, newState, req);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);


	lddChan.GetResourceStateAsync(ClientId[0], Resources[aResId].iResourceId, EFalse, req, state, levelOwnerId);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);
	test(state == newState);
	test(levelOwnerId == (TInt)ClientId[1]);
	
	r = lddChan.DeRegisterClient(ClientId[1]);
	test(r == KErrNone);
     
	r = lddChan.DeRegisterClientLevelFromResource(ClientId[0], Resources[aResId].iResourceId);
	test(r == KErrNone);

	r = lddChan.GetResourceStateSync(ClientId[0], Resources[aResId].iResourceId, EFalse, state, levelOwnerId);
	test(r == KErrNone);
	test(state == Resources[aResId].iDefaultLevel);
	test(levelOwnerId == -1);

	r = lddChan.DeRegisterClient(ClientId[0]);
	test(r == KErrNone);

	return;
	}

//Custom resource testing. This testing is done only with simulated resources.
//Testing of shared binary positive resource.
//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID	  KBASE-T_RESCONTROLCLI-0593
//! @SYMTestType		UT
//! @SYMPREQ			PREQ1398
//! @SYMTestCaseDesc	This test case tests changing resource state of shared positive resource.
//! @SYMTestActions		0	Register client1
//!						1	Register client2
//!						2	Register client3
//!						3	Register client4
//!						4	Client1 change resource state.
//!						5	Client2 change resource state.
//!						6	Client3 get resource state.
//!						7	Client4 get resource state.
//!						8	Client1 change resource state.
//!						9	Client2 get resource state.
//!						10	Deregister client2
//!						11	Deregister client1
//!						12	Deregister client3
//!						13	Deregister client4
//!
//! @SYMTestExpectedResults   0	Client registered
//!							  1	Client registered
//!							  2 Client registered
//!							  3	Client registered
//!							  4 Resource state changed
//!							  5	Resource state changed
//!							  6	Resource state read and compared for correctness
//!							  7 Resource state read and compared for correctness
//!							  8 Resource state changed
//!							  9 Resource state read and compared for correctness
//!							 10 Client2 deregistered
//!							 11 Client1 deregistered
//!							 12 Client3 deregistered
//!							 13 Client4	deregistered	
//! @SYMTestPriority		High
//! @SYMTestStatus		  Implemented
//----------------------------------------------------------------------------------------------
void TestRM::CustomResourceTesting(TUint aResId)
	{
	test.Printf(_L("Testing custom function\n"));
	TInt r = KErrNone;
	TRequestStatus req;
	TInt state;
	TInt newState;
	TInt levelOwnerId;
	TUint ClientId[4];
	ClientName[6] = (TUint8)('0' + iMaxClientId +1);
	r = lddChan.RegisterClient(ClientId[0], (const TDesC*)&ClientName);
	test(r == KErrNone);
	ClientName[6] = (TUint8)('0' + iMaxClientId +2);
	r = lddChan.RegisterClient(ClientId[1], (const TDesC*)&ClientName);
	test(r == KErrNone);
	ClientName[6] = (TUint8)('0' + iMaxClientId +3);
	r = lddChan.RegisterClient(ClientId[2], (const TDesC*)&ClientName);
	test(r == KErrNone);
	ClientName[6] = (TUint8)('0' + iMaxClientId +4);
	r = lddChan.RegisterClient(ClientId[3], (const TDesC*)&ClientName);
	test(r == KErrNone);
	r = lddChan.RequestNotification(ClientId[0], Resources[aResId].iResourceId);
	test(r == KErrNone);
	r = lddChan.RequestNotification(ClientId[1], Resources[aResId].iResourceId);
	test(r == KErrNone);
	newState = 1;
	state = 1;
	lddChan.ChangeResourceStateAsync(ClientId[2], Resources[aResId].iResourceId, state, req);
	User::WaitForRequest(req); //State 1
	test(req.Int() == KErrNone);
	test(state == newState);
	test(r == KErrNone);
	r = lddChan.CheckNotifications(Resources[aResId].iResourceId, 2, 0);
	test(r == KErrNone);
	state = 0;
	lddChan.ChangeResourceStateAsync(ClientId[1], Resources[aResId].iResourceId, state, req);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);
	test(state == newState);
	r = lddChan.GetResourceStateSync(ClientId[0], Resources[aResId].iResourceId, EFalse, state, levelOwnerId);
	test(r == KErrNone);
	test(state == 1);
	lddChan.ChangeResourceStateAsync(ClientId[0], Resources[aResId].iResourceId, state, req);
	User::WaitForRequest(req);
	test(req.Int() == KErrNone);
	test(state == newState);
	r = lddChan.GetResourceStateSync(ClientId[0], Resources[aResId].iResourceId, ETrue, state, levelOwnerId);
	test(r == KErrNone);
	test((TUint)levelOwnerId == ClientId[2]);
	r = lddChan.ChangeResourceStateSync(ClientId[2], Resources[aResId].iResourceId, 0);
	test(r == KErrNone);
	r = lddChan.GetResourceStateSync(ClientId[0], Resources[aResId].iResourceId, EFalse, state, levelOwnerId);
	test(r == KErrNone);
	test(state == 1);
	test((TUint)levelOwnerId == ClientId[0]);
	r = lddChan.DeRegisterClient(ClientId[0]);
	test(r == KErrNone);
	r = lddChan.DeRegisterClientLevelFromResource(ClientId[1], Resources[aResId].iResourceId);
	test(r == KErrNone);
	r= lddChan.DeRegisterClient(ClientId[2]);
	test(r == KErrNone);
	r = lddChan.GetResourceStateSync(ClientId[3], Resources[aResId].iResourceId, ETrue, state, levelOwnerId);
	test(r == KErrNone);
	test(levelOwnerId == -1);
	r = lddChan.DeRegisterClient(ClientId[3]);
	test(r == KErrNone);
	r = lddChan.DeRegisterClient(ClientId[1]);
	test(r == KErrNone);
	return;
	}

//Resource manager operations are chosen randomly and tested for correctness. This is done only in 
//simulated resources. Currently this runs for 500 iteration.
//NOTE: Increasing the iteration to more than 500 may fail due to insufficient internal buffers.
void TestRM::RegressionTest()
	{
	TUint operation = 0;
	TUint resourceId;
	TUint count;
	NegativeTesting = 0;
	iMaxClientId = 0;
	iMaxStaticResources = 0;
	iMaxStaticDependentResources = 0;
	iMaxClients = 0;
	RmTest.RegisterClient();
	iCurrentClientId = -1;
	r = lddChan.GetResourceControllerVersion(Clients[0].iClientId, iTestingExtendedVersion);
	if(r != KErrNone)
		test.Printf(_L("Return value of GetResourceControllerVersion %d\n"), r);
	test(r == KErrNone);
	if(!iTestingExtendedVersion)
		test.Printf(_L("Testing Basic Version only...."));
	else
		test.Printf(_L("Testing basic & extended version...."));
	RmTest.GetNumResourcesInUseByClient(iCurrentClientId);
	RmTest.GetInfoOnResourcesInUseByClient(iCurrentClientId, iMaxStaticResources);
   if(!(Resources[0].iName.Compare(*(const TDesC8*)&SpecialResName)))
		{
		TBuf8<32> PowerController = _L8("PowerController");
		r = lddChan.GetClientId(Clients[0].iClientId, (TDesC8&)PowerController, iPowerControllerId);
		test(r == KErrNone);
		}
	else
		{
		test.Printf(_L("Regression testing is run only on simulator"));
		return;
		}
	   
	for(count = 0; count < 500; count++)
		{
		operation = Math::Random() % EOperationEnd;
		iCurrentClientId = Math::Random() % iMaxClients;
		resourceId = Math::Random() % iMaxStaticResources;
		if(operation != ERegisterClient)
			{
			if(Clients[iCurrentClientId].iClientId == 0) //Not a valid client
				continue;
			}
		if(Resources[resourceId].iSense == ECustom)
			continue;
		test.Printf(_L("\nOperation = %d, ClientId = %d, ResourceId = %d\n"), operation, iCurrentClientId, resourceId);
		switch (operation)
			{
			case  ERegisterClient:
				RmTest.RegisterClient();
				break;
			case EGetClientName:
				RmTest.GetClientName(iCurrentClientId);
				break;
			case EGetAllClientName:
				RmTest.GetClientName(0);
				break;
			case EGetClientId:
				RmTest.GetClientId(iCurrentClientId);
				break;
			case EGetResourceId:
				RmTest.GetResourceId(resourceId);
				break;
			case EGetResourceInfo:
				RmTest.GetResourceInfo(resourceId);
				break;
			case EGetNumReosourceInUseByClient:
				RmTest.GetNumResourcesInUseByClient(iCurrentClientId);
				break;
			case EGetInfoOnResourceInUseByClient:
				test.Printf(_L("NumResources = %d\n"), Clients[iCurrentClientId].iNumResources);
				RmTest.GetInfoOnResourcesInUseByClient(iCurrentClientId, Clients[iCurrentClientId].iNumResources);
				break;
			case EGetNumClientsUsingResource:
				if(resourceId == 0)
					{
					RmTest.GetNumClientsUsingResource(iCurrentClientId, (TUint)-1);
					}
				else
					{
					RmTest.GetNumClientsUsingResource(iCurrentClientId, resourceId);
					}
				break;
			case EGetInfoOnClientsUsingResource:
				if(resourceId == 0)
					RmTest.GetInfoOnClientsUsingResource((TUint)-1, iMaxClients+1);
				else
					{
					test.Printf(_L("NumResources = %d\n"), Resources[resourceId].iNumClients);
					RmTest.GetInfoOnClientsUsingResource(resourceId, Resources[resourceId].iNumClients);
					}
				break;
			case EChangeResourceStateSync:
				RmTest.ChangeResourceStateSync(resourceId);
				break;
			case EChangeResourceStateAsync:
				RmTest.ChangeResourceStateAsync(resourceId);
				break;
			case EGetResourceStateSync:
				RmTest.GetResourceStateSync(resourceId);
				break;
			case EGetResourceStateAsync:
				RmTest.GetResourceStateAsync(resourceId);
				break;
			case ERequestNotificationCond:
				RmTest.RequestNotificationCon(resourceId);
				break;
			case ERequestNotificationUnCond:
				RmTest.RequestNotification(resourceId);
				break;
			case ECancelNotificationCond:
				RmTest.CancelNotification(resourceId, ETrue);
				break;
			case ECancelNotificationUnCond:
				RmTest.CancelNotification(resourceId, EFalse);
				break;
		}
	}
	//CleanUp
	test.Printf(_L("Cleanup of all Clients\n"));
	TInt clientCount = Clients.Count();
	for(count = clientCount-1; ((TInt)count) >=0; count--)
		{
		if(Clients[count].iClientId == 0)
			continue;
		test.Printf(_L("ClientId deregistration of %d\n"), Clients[count].iClientId);
		RmTest.DeRegisterClient(count);
	   }
	Clients.Close();
	}

void TestClientSettings(TInt aClientToken, TUint aExpectedBase)
	{
	TUint clientBase = ElementId_ClientSettingBase(aClientToken);
	test(clientBase == aExpectedBase);

	HCR::TElementId Id;

	Id = ElementId_ClientName(aClientToken);
	test(Id == aExpectedBase);

	Id = ElementId_ClientPropertyFlag(aClientToken);
	test(Id == (aExpectedBase + 1));

	Id = ElementId_ClientPreallocation(aClientToken);
	test(Id == (aExpectedBase + 2));

	TInt firstResource = 0;
	Id = ElementId_ClientStaticResource(aClientToken, firstResource);
	test(Id == (aExpectedBase + 3));
	
	TInt thirdResource = 2;
	Id = ElementId_ClientStaticResource(aClientToken, thirdResource);
	test(Id == (aExpectedBase + 5));
	}
	
void TestDynamicResourceSettings(TInt aDynamicResource,  TUint aExpectedBase)
	{
	TUint dynamicResourceBase = ElementId_DynamicResourceBase(aDynamicResource);
	test(dynamicResourceBase == aExpectedBase);
	
	HCR::TElementId Id;	
	
	Id = ElementId_DynamicResourceName(aDynamicResource);
	test(Id == aExpectedBase);

	Id = ElementId_DynamicResourcePropertyFlag(aDynamicResource);
	test(Id == (aExpectedBase + 1));

	Id = ElementId_DynamicResourceMaxLevel(aDynamicResource);
	test(Id == (aExpectedBase + 2));
	
	Id = ElementId_DynamicResourceMinLevel(aDynamicResource);
	test(Id == (aExpectedBase + 3));

	Id = ElementId_DynamicResourceDefaultLevel(aDynamicResource);
	test(Id == (aExpectedBase + 4));

	Id = ElementId_DynamicResourceDependencyMask1(aDynamicResource);
	test(Id == (aExpectedBase + 5));

	Id = ElementId_DynamicResourceDependencyMask2(aDynamicResource);
	test(Id == (aExpectedBase + 6));

	Id = ElementId_DynamicResourceDependencyMask3(aDynamicResource);
	test(Id == (aExpectedBase + 7));
	}
	
void TestClientHCRSettingMacros()
	{
	TInt clientToken = 2; // Random token number
	TUint expectedBase = 80; // refer resourcecontrol_clientsettings.h for calculations
	TestClientSettings(clientToken, expectedBase);
	
	clientToken = 0;
	expectedBase = 16; // refer resourcecontrol_clientsettings.h for calculations
	TestClientSettings(clientToken, expectedBase);

	// Dynamic Resource settings
	TUint dynamicResource = 3; // Random token number
	expectedBase = 131168; // refer resourcecontrol_clientsettings.h for calculations
	TestDynamicResourceSettings(dynamicResource, expectedBase);
	
	test.Printf(_L("Testing HCR client setting Macro's for Resource Manager successful \n"));
	}
	
GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Testing Resource Manager...\n"));
	test.Printf(_L("Testing HCR client setting Macro's for Resource Manager \n"));
	TestClientHCRSettingMacros();
	test.Next(_L("Load Physical device"));
#ifndef PRM_ENABLE_EXTENDED_VERSION
	r = User::LoadPhysicalDevice(KPddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);
	test.Next(_L("Load Logical Device"));
	r=User::LoadLogicalDevice(KLddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);
	__KHEAP_MARK; //Heap testing is done only for basic version
#else
	r = User::LoadPhysicalDevice(KExtPddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);
	test.Next(_L("Load Logical Device"));
	r=User::LoadLogicalDevice(KExtLddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);
#endif

	RDevice d;
	TPckgBuf<RTestResMan::TCaps> caps;
	r = d.Open(KLddName);
	test(r == KErrNone);
	d.GetCaps(caps);
	d.Close();

	TVersion ver = caps().iVersion;
	test(ver.iMajor == 1);
	test(ver.iMinor == 0);
	test(ver.iBuild == KE32BuildVersionNumber);

	r = lddChan.Open();
	test(r==KErrNone || r==KErrAlreadyExists);

	//Check whether the notifications recieved as a result of postboot level setting is as expected.
	r = lddChan.CheckPostBootLevelNotifications();
	test(r == KErrNone);
	
	//Test resource controller registration
	r = lddChan.CheckResourceControllerRegistration();
	test(r == KErrNone);

	TBool regressionTesting = EFalse;
	//Parse the command line arguments.
	TBuf<0x50> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	lex.SkipSpace();
	if(lex.Get() == '-')
		{
		TChar letter = lex.Get();
		if((letter == 'R') || (letter == 'r'))
			regressionTesting = ETrue;
		}
	if(regressionTesting)
		RmTest.RegressionTest();
	else
		RmTest.APIValidationTest();
	test.Printf(_L("Closing the channel\n"));
	lddChan.Close();
	test.Printf(_L("Freeing logical device\n"));
#ifndef PRM_ENABLE_EXTENDED_VERSION
	__KHEAP_MARKEND;
	r = User::FreeLogicalDevice(KLddFileName);
	test(r==KErrNone);
	r = User::FreePhysicalDevice(KPddFileName);
	test(r==KErrNone);
#else
	r = User::FreeLogicalDevice(KExtLddFileName);
	test(r==KErrNone);
	r = User::FreePhysicalDevice(KExtPddFileName);
	test(r==KErrNone);
#endif
	User::After(100000);
	test.End();
	test.Close();
	return KErrNone;
	}

