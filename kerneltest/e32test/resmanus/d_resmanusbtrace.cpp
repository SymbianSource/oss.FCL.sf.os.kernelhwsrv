// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\resmanus\d_resmanusbtrace.cpp
// 
//

#include <kernel/kernel.h>
#include <drivers/resource.h>
#include <drivers/resourcecontrol.h>
#include <drivers/resmanus_trace.h>
#include <drivers/resourcecontrol_trace.h>
#include "d_resmanusbtraceconst.h"
#include "d_resmanusbtrace.h"

class DTestFactory : public DLogicalDevice
//
// Test LDD factory
//
	{
public:
	DTestFactory();
	virtual TInt Install(); 					//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel); 	//overriding pure virtual
	};

class DTest1 : public DLogicalChannelBase
//
// Test logical channel
//
	{
public:
	virtual ~DTest1();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);

private:
	TInt DoSendLog(TLogInfo* aInfo);
	TInt ClientHandle() { return KClientHandle; }
	};



DECLARE_STANDARD_LDD()
	{
	return new DTestFactory;
	}

//
// Constructor
//
DTestFactory::DTestFactory()
	{

	}

TInt DTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
//
// Create new channel
//
	aChannel=new DTest1;
	return aChannel?KErrNone:KErrNoMemory;
	}

TInt DTestFactory::Install()
//
// Install the LDD - overriding pure virtual
//
	{
	return SetName(&KLddName);
	}

void DTestFactory::GetCaps(TDes8& /*aDes*/) const
//
// Get capabilities - overriding pure virtual
//
	{
	}

TInt DTest1::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
//
// Create channel
//
	{
	return KErrNone;
	}

DTest1::~DTest1()
//
// Destructor
//
	{
	}

TInt DTest1::Request(TInt aReqNo, TAny* a1, TAny* /*a2*/)
	{

	// 'Control' functions...
	switch(aReqNo)
		{
		// DoControl
		case RLddTest1::ECONTROL_SENDLOG:
		    TLogInfo info;
			DoSendLog(&info);
			Kern::ThreadRawWrite(&Kern::CurrentThread(), a1, &info, sizeof(info));
			break;
		}

	return KErrNone;
	}
	
class TestResource 
    {
public:
    TestResource(const TDesC8& aName, TInt aDefaultLevel)
        {
        iName = (HBuf8*)&aName;
        iDefaultLevel = aDefaultLevel;
        }
    HBuf* iName;
    TInt iDefaultLevel;
    TInt iResourceId;
    };

struct TestClient
    {
    TDes8* iName;
    TInt iClientId;
    };

struct TestCallback
    {
    TInt iResourceId;
    TInt iClientId;
    };

struct TestNotification
    {
    TestCallback iCallback;
    };

class TestRequest
    {
public:
    TInt ResourceId() {return iResourceId;}
    TInt ClientId() {return iClientId;}
    TInt Level() {return iLevel;}
    TInt iResourceId;
    TInt iClientId;
    TInt iLevel;
    };

TInt DTest1::DoSendLog(TLogInfo* aInfo)
	{
	TInt r = KErrNoMemory;
	
	TInt iClient = (TInt) KClient;
	TBuf<80> Buffer;
	TDes8* iUserNameUsed = &Buffer;
	Buffer.Append(KCLIENTNAME);
	Kern::Printf("PRM_US_OPEN_CHANNEL_START_TRACE");
	PRM_US_OPEN_CHANNEL_START_TRACE
	
	Kern::Printf("PRM_US_OPEN_CHANNEL_END_TRACE %x", (TInt)ClientHandle());
	PRM_US_OPEN_CHANNEL_END_TRACE
	
	TUint8 stateRes[3] = {KStatsRes1, KStatsRes2, KStatsRes3};
	Kern::Printf("PRM_US_REGISTER_CLIENT_START_TRACE");
	PRM_US_REGISTER_CLIENT_START_TRACE

	Kern::Printf("PRM_US_REGISTER_CLIENT_END_TRACE");
	PRM_US_REGISTER_CLIENT_END_TRACE
	
	Kern::Printf("PRM_US_DEREGISTER_CLIENT_START_TRACE");
	PRM_US_DEREGISTER_CLIENT_START_TRACE
	
    Kern::Printf("PRM_US_DEREGISTER_CLIENT_END_TRACE");
	PRM_US_DEREGISTER_CLIENT_END_TRACE	
	
	TUint resourceId = KResourceId;
    Kern::Printf("PRM_US_GET_RESOURCE_STATE_START_TRACE");
	PRM_US_GET_RESOURCE_STATE_START_TRACE
	
	TUint aResourceId = KResourceId;
	TUint aLevel = KLevel;
	TUint aClient = KClient;
	TUint aResult = KResult;
    Kern::Printf("PRM_US_GET_RESOURCE_STATE_END_TRACE");
	PRM_US_GET_RESOURCE_STATE_END_TRACE
	
	TUint newState = KLevel;
    Kern::Printf("PRM_US_SET_RESOURCE_STATE_START_TRACE");
	PRM_US_SET_RESOURCE_STATE_START_TRACE
	
    Kern::Printf("PRM_US_SET_RESOURCE_STATE_END_TRACE");
	PRM_US_SET_RESOURCE_STATE_END_TRACE
	
	Kern::Printf("PRM_US_CANCEL_GET_RESOURCE_STATE_START_TRACE");
	PRM_US_CANCEL_GET_RESOURCE_STATE_START_TRACE
	
	Kern::Printf("PRM_US_CANCEL_GET_RESOURCE_STATE_END_TRACE");
	PRM_US_CANCEL_GET_RESOURCE_STATE_END_TRACE
	
	Kern::Printf("PRM_US_CANCEL_SET_RESOURCE_STATE_START_TRACE");
	PRM_US_CANCEL_SET_RESOURCE_STATE_START_TRACE
	
	Kern::Printf("PRM_US_CANCEL_SET_RESOURCE_STATE_END_TRACE");
	PRM_US_CANCEL_SET_RESOURCE_STATE_END_TRACE
	
	TPowerResourceInfoV01 ResourceInfo;
	TPowerResourceInfoV01* pResInfo = &ResourceInfo;
	TestResource Resource(KRESOURCENAME, KDefaultLevel);
	TestResource* pR = &Resource;
	pResInfo->iMinLevel = KMinLevel;
	pResInfo->iMaxLevel = KMaxLevel;
	pResInfo->iDefaultLevel = KDefaultLevel;
	TUint resCount = KResCount;
    Kern::Printf("PRM_REGISTER_RESOURCE_TRACE");
	PRM_REGISTER_RESOURCE_TRACE
	
	TUint aClientId = KClientId;
	TBuf8<80> ClientName(KCLIENTNAME);
	SPowerResourceClient ResourceClient;
	SPowerResourceClient* pC = &ResourceClient;
	pC->iName = &ClientName;
    Kern::Printf("PRM_CLIENT_REGISTER_TRACE");
	PRM_CLIENT_REGISTER_TRACE
	
    Kern::Printf("PRM_CLIENT_DEREGISTER_TRACE");
	PRM_CLIENT_DEREGISTER_TRACE
	
	TInt aNewState = KLevel;
	pC->iClientId = KClientId;
	Kern::Printf("PRM_CLIENT_CHANGE_STATE_START_TRACE");
	PRM_CLIENT_CHANGE_STATE_START_TRACE

	pC->iClientId = KClientId;
	Kern::Printf("PRM_CLIENT_CHANGE_STATE_END_TRACE");
	PRM_CLIENT_CHANGE_STATE_END_TRACE
	
	TestNotification aN;
	TestNotification* pN = &aN;
	pN->iCallback.iResourceId = KResourceId;
	
    Kern::Printf("PRM_POSTNOTIFICATION_REGISTER_TRACE");
	PRM_POSTNOTIFICATION_REGISTER_TRACE

    Kern::Printf("PRM_POSTNOTIFICATION_DEREGISTER_TRACE");
	PRM_POSTNOTIFICATION_DEREGISTER_TRACE
	
    Kern::Printf("PRM_POSTNOTIFICATION_SENT_TRACE");
	PRM_POSTNOTIFICATION_SENT_TRACE
	
	TestCallback* pCb = &(pN->iCallback);
	pCb->iClientId = KClientId;
    Kern::Printf("PRM_CALLBACK_COMPLETION_TRACE");
	PRM_CALLBACK_COMPLETION_TRACE
	
	TInt size = KSize;
	PRM_MEMORY_USAGE_TRACE
	
	TestRequest aRequest;
	aRequest.iClientId = KClientId;
	aRequest.iResourceId = KResourceId;
	
	TDesC8* iName = iUserNameUsed;
    Kern::Printf("PRM_PSL_RESOURCE_GET_STATE_START_TRACE");
	PRM_PSL_RESOURCE_GET_STATE_START_TRACE
	
    Kern::Printf("PRM_RESOURCE_GET_STATE_START_TRACE");
	PRM_RESOURCE_GET_STATE_START_TRACE
	
	TInt retVal = KRetVal;
	TInt iCurLevel = KLevel;
	TInt aState = KLevel;
    Kern::Printf("PRM_PSL_RESOURCE_GET_STATE_END_TRACE");
	PRM_PSL_RESOURCE_GET_STATE_END_TRACE
	
    Kern::Printf("PRM_RESOURCE_GET_STATE_END_TRACE");
	PRM_RESOURCE_GET_STATE_END_TRACE
	
    Kern::Printf("PRM_RESOURCE_CANCEL_LONGLATENCY_OPERATION_TRACE");
	PRM_RESOURCE_CANCEL_LONGLATENCY_OPERATION_TRACE
	
	aRequest.iLevel = KLevel;
    Kern::Printf("PRM_PSL_RESOURCE_CHANGE_STATE_START_TRACE");
	PRM_PSL_RESOURCE_CHANGE_STATE_START_TRACE
	
    Kern::Printf("PRM_PSL_RESOURCE_CHANGE_STATE_END_TRACE");
	PRM_PSL_RESOURCE_CHANGE_STATE_END_TRACE
	
	TInt iDefaultLevel = KDefaultLevel;
	TInt iFlags = KFlags;
	TInt iMinLevel = KMinLevel;
	TInt iMaxLevel = KMaxLevel;
	TInt aReason = KErrNoMemory;
    Kern::Printf("PRM_PSL_RESOURCE_CREATE_TRACE");
	PRM_PSL_RESOURCE_CREATE_TRACE
	
    Kern::Printf("PRM_BOOTING_TRACE");
	PRM_BOOTING_TRACE
	
	TestResource* aPDRes = &Resource;
	aPDRes->iResourceId = KResourceId;
	TestClient Client;
	TestClient* aClientPtr = &Client;
	aClientPtr->iName = &ClientName;
	aClientPtr->iClientId = KClientId;
    Kern::Printf("PRM_REGISTER_STATIC_RESOURCE_WITH_DEPENDENCY_TRACE");
	PRM_REGISTER_STATIC_RESOURCE_WITH_DEPENDENCY_TRACE

	TestResource* pDR = &Resource;
	TInt level = KLevel;
    Kern::Printf("PRM_REGISTER_DYNAMIC_RESOURCE_TRACE");
	PRM_REGISTER_DYNAMIC_RESOURCE_TRACE
	
    Kern::Printf("PRM_DEREGISTER_DYNAMIC_RESOURCE_TRACE");
	PRM_DEREGISTER_DYNAMIC_RESOURCE_TRACE
	
	TestResource* pR1 = &Resource;
	TestResource* pR2 = &Resource;
    Kern::Printf("PRM_REGISTER_RESOURCE_DEPENDENCY_TRACE");
	PRM_REGISTER_RESOURCE_DEPENDENCY_TRACE
	
	TestResource* pDR1 = &Resource;
	TestResource* pDR2 = &Resource;
    Kern::Printf("PRM_DEREGISTER_RESOURCE_DEPENDENCY_TRACE");
	PRM_DEREGISTER_RESOURCE_DEPENDENCY_TRACE
	
	aInfo->iPR = &Resource;
	aInfo->iPC = &ResourceClient;
	aInfo->iPN = pN;
	aInfo->iPCb = pCb;
	aInfo->iPClient = &Client;
	aInfo->iPCallback = &(pN->iCallback);
	
	return KErrNone;
	}
